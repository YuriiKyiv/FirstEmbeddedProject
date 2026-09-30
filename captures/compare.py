"""Порівняння фронтів з Saleae (logic.csv) і спадів, які порахував ESP32
(serial_old.txt, прошивка з FALLING і групуванням за паузою 150 мс)."""
import csv, re

# --- аналізатор: групуємо фронти в натискання (лінія 1 і тиша > 50 мс) ---
rows = []
with open('captures/logic_old.csv') as f:
    r = csv.reader(f); next(r)
    for t, l in r: rows.append((float(t), int(l)))
edges = rows[1:]
la = []; cur = []
for i, (t, l) in enumerate(edges):
    cur.append((t, l))
    nxt = edges[i + 1][0] if i + 1 < len(edges) else None
    if l == 1 and (nxt is None or nxt - t > 0.050):
        la.append(cur); cur = []

# --- плата: список (група_логу, мкс відносно початку групи) у порядку появи ---
mcu = []
for gi, line in enumerate(l for l in open('captures/serial_old.txt') if 'edges(us)' in l):
    mcu += [(gi, int(x)) for x in line.split(':')[1].split()]

# --- зіставлення в порядку появи. Час усередині однієї групи логу точний,
#     між групами невідомий: стара прошивка починала нову групу після 150 мс тиші ---
p = 0
print(f"{'#':>2} | {'LA фронтів':>10} {'LA спадів':>9} | {'MCU спадів':>10} | спади MCU -> найближчий фронт LA (мкс від початку натискання)")
tot = [0, 0, 0]
for k, g in enumerate(la, 1):
    t0 = g[0][0]; la_rel = [round((t - t0) * 1e6) for t, _ in g]
    span = la_rel[-1]
    gi, base = mcu[p]; matched = [(0, 0)]; p += 1; offset = 0
    while p < len(mcu):
        g2, u = mcu[p]
        if g2 == gi:
            rel = u - base + offset
            if rel > span + 2000: break
        else:
            # нова група логу: припустима лише якщо в LA є ще фронт пізніше за 150 мс від початку
            rest = [x for x in la_rel if x > matched[-1][0] and x > 150000]
            if not rest: break
            gi, base, offset = g2, u, rest[0]; rel = rest[0]
        matched.append((rel, u)); p += 1
    pairs = []
    for rel, _ in matched:
        near = min(la_rel, key=lambda x: abs(x - rel))
        lvl = 'v' if g[la_rel.index(near)][1] == 0 else '^'
        pairs.append(f"{rel}->{lvl}{near}")
    la_f = sum(1 for _, l in g if l == 0)
    tot[0] += len(g); tot[1] += la_f; tot[2] += len(matched)
    print(f"{k:>2} | {len(g):>10} {la_f:>9} | {len(matched):>10} | {' '.join(pairs)}")
print(f"\nРазом: фронтів LA = {tot[0]}, спадів LA = {tot[1]}, спадів MCU = {tot[2]}, не зіставлено спадів MCU = {len(mcu) - p}")
