"""Why does a sample R5 vertical home fail? Print the offending checks/obstacles."""
import math

from shapely.geometry import box
from shapely.strtree import STRtree

import repaired_hub as R

m = R.copper_model(R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2')
MODULE = box(500, 30, 2460, 1040)
inner = m['outline'].buffer(-16)
obstacles = [i for i in m['items'] if i['net'] != 'SERVO_IN' and not i['label'].startswith('R5.')]
holes = [h for h in m['holes'] if not str(h['des']).startswith('R5.')]
obs = [i['shape'] for i in obstacles] + [h['shape'] for h in holes]
lbl = [i['label'] for i in obstacles] + [f'hole {h["des"]}.{h["num"]}' for h in holes]
lay = [(set((1, 2)) if i['kind'] == 'via' else set(i['layers'])) for i in obstacles] + [{1, 2}] * len(holes)
tree = STRtree(obs)

for cx, cy in [(1990, 2000), (2100, 2200), (2300, 2400), (1200, 2000), (700, 2200),
               (3200, 2500), (3600, 2600), (-300, 2200), (1990, 3200)]:
    print(f'\n=== home ({cx},{cy}) rot90 ===')
    body = box(cx - 90, cy - 263, cx + 90, cy + 263)
    print(' module:', MODULE.intersects(body), ' inner:', not inner.covers(body))
    worst = sorted(((body.distance(obs[int(j)]), lbl[int(j)], lay[int(j)]),
                    ) for j in tree.query(body.buffer(20)))
    near = sorted(((body.distance(obs[int(j)]), lbl[int(j)], sorted(lay[int(j)]))
                   for j in tree.query(body.buffer(40))))[:6]
    print(' nearest obstacles (dist, label, layers):', near)
    for sgn, pin in ((-1, 1), (1, 2)):
        px, py = cx, cy + sgn * 202.8
        pad = box(px - 25.6, py - 25.6, px + 25.6, py + 25.6).buffer(6)
        near = sorted(((pad.distance(obs[int(j)]), lbl[int(j)], sorted(lay[int(j)]))
                       for j in tree.query(pad.buffer(40))))[:5]
        print(f' pad{pin} ({px},{py}) nearest:', near)
