"""Verbose single-point probe: why does home (1890,1335,rot0) fail now?"""
import math

from shapely.geometry import box
from shapely.strtree import STRtree

import repaired_hub as R

m = R.copper_model(R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2')
MODULE = box(500, 30, 2460, 1040)
inner = m['outline'].buffer(-16)
PAD_SIDE, PAD_OFF, W = 51.18, 202.8, 13.7795
obstacles = [i for i in m['items'] if i['net'] != 'SERVO_IN' and not i['label'].startswith('R5.')]
holes = [h for h in m['holes'] if not str(h['des']).startswith('R5.')]
snap = R.json_value(R.DATA / 'repaired_silk_positioned.json')
silk = [box(q['box']['minX'], q['box']['minY'], q['box']['maxX'], q['box']['maxY'])
        for q in snap['texts'] + snap['designators'] if q.get('box') and q['text'] != 'R5']
obs = [i['shape'] for i in obstacles] + [h['shape'] for h in holes]
lbl = [i['label'] for i in obstacles] + [f'hole {h["des"]}.{h["num"]}' for h in holes]
meta = [({1, 2} if i['kind'] == 'via' else set(i['layers'])) for i in obstacles] + [{1, 2}] * len(holes)
pad_like = [i['kind'] == 'pad' for i in obstacles] + [False] * len(holes)
tree, stree = STRtree(obs), STRtree(silk)

cx, cy, rot = 1890, 1335, 0
pads = [(cx - PAD_OFF, cy), (cx + PAD_OFF, cy)]
body = box(cx - PAD_OFF - 60, cy - 90, cx + PAD_OFF + 60, cy + 90)
print('module', MODULE.intersects(body), 'inner', not inner.covers(body))
print('body vs pads/holes:', [(round(body.distance(obs[int(j)]), 1), lbl[int(j)])
                              for j in tree.query(body.buffer(6)) if pad_like[int(j)] and body.distance(obs[int(j)]) < 6])
for k, (px, py) in enumerate(pads):
    pb = box(px - PAD_SIDE / 2, py - PAD_SIDE / 2, px + PAD_SIDE / 2, py + PAD_SIDE / 2).buffer(6)
    bad = [(round(pb.distance(obs[int(j)]), 2), lbl[int(j)]) for j in tree.query(pb.buffer(4))
           if meta[int(j)] & {1, 2} and pb.distance(obs[int(j)]) < 6]
    print(f'pad{k+1}', pads[k], 'violations:', bad)
print('silk on body:', [q['text'] for q in snap['texts'] + snap['designators'] if q.get('box')
                        and body.intersects(box(q['box']['minX'] - 6, q['box']['minY'] - 6,
                                                q['box']['maxX'] + 6, q['box']['maxY'] + 6))])
own = [box(px - PAD_SIDE / 2, py - PAD_SIDE / 2, px + PAD_SIDE / 2, py + PAD_SIDE / 2) for px, py in pads]
for ddx, ddy in ((-160, 0), (160, 0), (0, -292.8), (0, 292.8),
                 (-160, -292.8), (160, -292.8), (-160, 292.8), (160, 292.8)):
    dbox = box(cx + ddx - 70, cy + ddy - 30, cx + ddx + 70, cy + ddy + 30)
    why = []
    if MODULE.intersects(dbox):
        why.append('module')
    if not inner.covers(dbox):
        why.append('edge')
    if any(dbox.distance(p) < 6 for p in own):
        why.append('ownpad')
    hits = [q['text'] for q in snap['texts'] + snap['designators'] if q.get('box')
            and dbox.intersects(box(q['box']['minX'] - 6, q['box']['minY'] - 6, q['box']['maxX'] + 6, q['box']['maxY'] + 6))]
    if hits:
        why.append('silk:' + ','.join(hits[:3]))
    cu = [(round(dbox.distance(obs[int(j)]), 1), lbl[int(j)]) for j in tree.query(dbox.buffer(6))
          if meta[int(j)] & {1, 2} and dbox.distance(obs[int(j)]) < 6]
    if cu:
        why.append(f'copper:{cu[:3]}')
    print(f'desig ({cx+ddx},{cy+ddy})', 'OK' if not why else why)
