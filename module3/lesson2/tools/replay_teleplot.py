#!/usr/bin/env python3
"""Replay the recorded board measurements into Teleplot over localhost UDP."""

import argparse
import csv
import json
from pathlib import Path
import socket
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--csv', type=Path, default=Path(__file__).resolve().parents[1]
                        / 'results' / 'measurements.csv')
    parser.add_argument('--speed', type=float, default=1.0,
                        help='Playback speed; 1 preserves the original timing')
    parser.add_argument('--export-json', type=Path,
                        help='Create an offline Teleplot session instead of sending UDP')
    args = parser.parse_args()
    if not 0 < args.speed < float('inf'):
        parser.error('--speed must be finite and greater than zero')
    with args.csv.open(newline='', encoding='utf-8') as source:
        rows = list(csv.DictReader(source))
    if not rows:
        parser.error('CSV has no measurements')
    if args.export_json:
        telemetries = {}
        definitions = [('RAW', 'code', False), ('U_manual_mV', 'mV', False),
                       ('U_cali_mV', 'mV', False), ('Error_pct', '%', False),
                       ('Manual_vs_RAW', 'mV', True), ('Cali_vs_RAW', 'mV', True),
                       ('Delta_mV_vs_RAW', 'mV', True), ('Error_vs_RAW', '%', True)]
        # Sort and deduplicate RAW for transfer curves; keep every time sample.
        by_raw = sorted({int(row['RAW']): row for row in rows}.values(),
                        key=lambda row: int(row['RAW']))
        for name, unit, xy in definitions:
            points = []
            for row in by_raw if xy else rows:
                raw, manual, cal = int(row['RAW']), float(row['U_manual_mV']), int(row['U_cali_mV'])
                if name.startswith('Error') and cal == 0:
                    continue
                values = {'RAW': raw, 'U_manual_mV': manual, 'U_cali_mV': cal,
                          'Manual_vs_RAW': manual, 'Cali_vs_RAW': cal,
                          'Delta_mV_vs_RAW': cal - manual,
                          'Error_pct': float(row['Error_pct']) if cal else None,
                          'Error_vs_RAW': float(row['Error_pct']) if cal else None}
                timestamp = int(row['t_ms']) / 1000
                points.append([raw, values[name], timestamp] if xy else [timestamp, values[name]])
            data = [list(column) for column in zip(*points)]
            telemetries[name] = dict(type='xy' if xy else 'number', name=name, unit=unit,
                                    usageCount=0, values=points[-1][:2] if xy else [points[-1][1]],
                                    data=data, pendingData=[[] for _ in data])
        session = dict(telemetries=telemetries, logs=[], dataAvailable=True, logAvailable=False)
        args.export_json.write_text(json.dumps(session, indent=2, allow_nan=False) + '\n')
        print(f'Exported {len(rows)} time samples and {len(by_raw)} distinct RAW codes to {args.export_json}')
        return

    start = time.monotonic()
    first_ms = int(rows[0]['t_ms'])
    # Keep the original spacing on the time axis even when replaying faster.
    epoch_ms = int(time.time() * 1000) - (int(rows[-1]['t_ms']) - first_ms)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        address = ('127.0.0.1', 47269)
        sock.sendto(b'>:REPLAY of recorded ESP32-S3 measurements; not live Serial\n'
                    b'>: RAW  U_manual(mV)  U_cali(mV)  Error(%)', address)
        last_raw = None
        for row in rows:
            offset_ms = int(row['t_ms']) - first_ms
            time.sleep(max(0, start + offset_ms / 1000 / args.speed - time.monotonic()))
            timestamp = epoch_ms + offset_ms
            raw = int(row['RAW'])
            manual = float(row['U_manual_mV'])
            calibrated = int(row['U_cali_mV'])
            messages = [
                f'RAW:{timestamp}:{raw}',
                f'U_manual_mV,Voltage_mV:{timestamp}:{manual:.1f}',
                f'U_cali_mV,Voltage_mV:{timestamp}:{calibrated}',
                f'>:{raw:4d} {manual:13.1f} {calibrated:11d} {row["Error_pct"]:>9}',
            ]
            if calibrated != 0:
                error = float(row['Error_pct'])
                messages.append(f'Error_pct:{timestamp}:{error:.2f}')
            if raw != last_raw:
                messages.extend([f'Manual_vs_RAW,Voltage_vs_RAW:{raw}:{manual:.1f}:{timestamp}|xy',
                                 f'Cali_vs_RAW,Voltage_vs_RAW:{raw}:{calibrated}:{timestamp}|xy',
                                 f'Delta_mV_vs_RAW:{raw}:{calibrated - manual:.1f}:{timestamp}|xy'])
                if calibrated:
                    messages.append(f'Error_vs_RAW:{raw}:{error:.2f}:{timestamp}|xy')
                last_raw = raw
            sock.sendto('\n'.join(messages).encode('utf-8'), address)
    print(f'Replayed {len(rows)} recorded samples from {args.csv}')


if __name__ == '__main__':
    main()
