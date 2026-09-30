"""Порівняння фронтів Saleae (logic.csv) з перериваннями ESP32 на CHANGE (serial2.txt).
Обидва джерела мають рівень після фронту: v = 0, ^ = 1.
Фронти аналізатора ближче за MERGE_US один до одного контролер фізично не розділяє
(латентність входу в ISR), тому вони рахуються як одна очікувана подія."""
import csv, re

MERGE_US = 4      # пара фронтів ближче за це = одна подія для контролера
TOL_US = 20       # допуск при зіставленні часу (годинники розходяться на ~50 ppm)

rows = []
with open('captures/logic.csv') as f:
    r = csv.reader(f); next(r)
    for t, l in r: rows.append((float(t), int(l)))
edges = rows[1:]
dts = [b[0] - a[0] for a, b in zip(edges, edges[1:])]
print(f"Аналізатор: фронтів {len(edges)}, найменший інтервал {min(dts)*1e6:.2f} мкс")
la = []; cur = []
for i, (t, l) in enumerate(edges):
    cur.append((t, l))
    nxt = edges[i + 1][0] if i + 1 < len(edges) else None
    if l == 1 and (nxt is None or nxt - t > 0.050):
        la.append(cur); cur = []

mcu = []
for line in open('captures/serial2.txt'):
    m = re.match(r'\s*edges\(us\):(.*)', line)
    if m:
        mcu.append([(int(x[1:]), 0 if x[0] == 'v' else 1) for x in m.group(1).split()])
print(f"Натискань: аналізатор {len(la)}, плата {len(mcu)}\n")

def sym(l): return 'v' if l == 0 else '^'
hdr = f"{'#':>2} | {'LA фр':>5} {'LA подій':>8} | {'MCU':>3} | {'збіг':>4} {'злиті':>5} {'зайві':>5} {'пропущ':>6} | тривалість брязкоту при відпусканні, мкс"
print(hdr)
T = dict(la=0, ev=0, mcu=0, ok=0, merged=0, extra=0, missed=0)
for k, (g, m) in enumerate(zip(la, mcu), 1):
    t0 = g[0][0]; la_e = [(round((t - t0) * 1e6), l) for t, l in g]
    # масштаб годинників: остання чиста пара фронтів (підйом) обох джерел
    scale = la_e[-1][0] / m[-1][0] if m[-1][0] else 1.0
    m_s = [(u * scale, l) for u, l in m]
    # злиття близьких фронтів у події: час першого, рівень останнього
    ev = []
    for u, l in la_e:
        if ev and u - ev[-1][0] < MERGE_US: ev[-1] = (ev[-1][0], l, ev[-1][2] + 1)
        else: ev.append((u, l, 1))
    merged = sum(n - 1 for _, _, n in ev)
    # послідовне зіставлення
    i = j = 0; ok = extra = missed = 0; notes = []
    while i < len(ev) or j < len(m_s):
        if i < len(ev) and j < len(m_s) and abs(ev[i][0] - m_s[j][0]) <= TOL_US:
            ok += 1
            if ev[i][1] != m_s[j][1]: notes.append(f"рівень {sym(m_s[j][1])}{m[j][0]} проти LA {sym(ev[i][1])}{ev[i][0]}")
            i += 1; j += 1
        elif j < len(m_s) and (i >= len(ev) or m_s[j][0] < ev[i][0]):
            extra += 1; notes.append(f"зайве переривання {sym(m[j][1])}{m[j][0]}"); j += 1
        else:
            missed += 1; notes.append(f"пропущено LA {sym(ev[i][1])}{ev[i][0]}"); i += 1
    rel = g[1:]
    bounce = round((rel[-1][0] - rel[0][0]) * 1e6) if len(rel) > 1 else 0
    for key, val in zip(T, [len(la_e), len(ev), len(m), ok, merged, extra, missed]): T[key] += val
    print(f"{k:>2} | {len(la_e):>5} {len(ev):>8} | {len(m):>3} | {ok:>4} {merged:>5} {extra:>5} {missed:>6} | {bounce:>5}   {'; '.join(notes)}")
print(f"\nРазом: LA фронтів {T['la']}, подій для контролера {T['ev']} (злитих пар {T['merged']}), "
      f"переривань MCU {T['mcu']}: збіглось {T['ok']}, зайвих {T['extra']}, пропущених {T['missed']}")
