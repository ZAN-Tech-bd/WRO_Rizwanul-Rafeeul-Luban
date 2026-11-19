"""R5 move plan v4: vertical homes only (clear U2 DIP body), full pad+body clearance checks.

Deletes the old SERVO_IN stub and taps the same net's bottom track with the shortest clean
straight/one-elbow path. Early-exit tap search; writes r5_move_plan.json.
"""
import json
import math

from shapely.geometry import LineString, box
from shapely.strtree import STRtree

import repaired_hub as R

ARCHIVE = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2'
MODULE = box(500, 30, 2460, 1040)
STUB_ID = '39b1d74c0d7108f7'
W = 13.7795
PAD_SIDE = 51.18
PAD_OFF = 202.8


def main():
    m = R.copper_model(ARCHIVE)
    snap = R.json_value(R.DATA / 'repaired_silk_positioned.json')
    obstacles = [i for i in m['items']
                 if i['net'] != 'SERVO_IN' and not i['label'].startswith('R5.')]
    holes = [h for h in m['holes'] if not str(h['des']).startswith('R5.')]
    silk_geoms = [box(q['box']['minX'], q['box']['minY'], q['box']['maxX'], q['box']['maxY'])
                  for q in snap['texts'] + snap['designators'] if q.get('box') and q['text'] != 'R5']
    inner = m['outline'].buffer(-16)

    lbl = [i['label'] for i in obstacles] + [f'hole {h["des"]}.{h["num"]}' for h in holes]
    obs = [i['shape'] for i in obstacles] + [h['shape'] for h in holes]
    meta = [(set((1, 2)) if i['kind'] == 'via' else set(i['layers'])) for i in obstacles] + \
           [{1, 2} for _ in holes]
    # a THT body only collides with top-layer copper, vias (top ring) and holes
    top_meta = [meta[k] if (meta[k] == {1, 2} or meta[k] == {1}) else set()
                for k in range(len(meta))]
    # body may sit over via holes (insulated body on mask); only foreign component pads collide
    is_pad_or_hole = [i['kind'] == 'pad' for i in obstacles] + [False] * len(holes)
    tree = STRtree(obs)
    silk_tree = STRtree(silk_geoms)

    def copper_ok(g, top_only=False):
        if not inner.covers(g):
            return False
        if g.buffer(8).intersects(m['antenna']):
            return False
        for j in tree.query(g.buffer(6)):
            j = int(j)
            rule = top_meta[j] if top_only else meta[j]
            if rule & {1, 2} and g.distance(obs[j]) < 6:
                return False
        return True

    taps = []
    for i in m['items']:
        if i['net'] != 'SERVO_IN' or i['id'] == STUB_ID:
            continue
        if i['kind'] == 'track':
            b = m['records'][('LINE', i['id'])]['body']
            if b['layerId'] != 2:
                continue
            a, z = (b['startX'], b['startY']), (b['endX'], b['endY'])
            n = max(1, int(math.hypot(z[0] - a[0], z[1] - a[1]) // 40))
            taps += [(a[0] + (z[0] - a[0]) * k / n, a[1] + (z[1] - a[1]) * k / n) for k in range(n + 1)]
        elif i['kind'] == 'via':
            taps.append(i['shape'].centroid.coords[0])
    taps = sorted(set((round(x, 1), round(y, 1)) for x, y in taps))
    print('taps:', len(taps))

    def path(p1, tgt):
        for pts in ([(p1, tgt)],
                    [(p1, (p1[0], tgt[1])), ((p1[0], tgt[1]), tgt)],
                    [(p1, (tgt[0], p1[1])), ((tgt[0], p1[1]), tgt)]):
            if all(copper_ok(LineString(s).buffer(W / 2, quad_segs=16)) for s in pts):
                return [[round(v, 3) for v in p] for s in pts for p in s]
        return None

    def stage(cx, cy, rot=90):
        """Return the name of the first failing stage for debugging."""
        shapes = []
        for sgn in (-1, +1):
            px, py = (cx + sgn * PAD_OFF, cy) if rot == 0 else (cx, cy + sgn * PAD_OFF)
            shapes.append(box(px - PAD_SIDE / 2, py - PAD_SIDE / 2,
                              px + PAD_SIDE / 2, py + PAD_SIDE / 2).buffer(6))
        body = box(cx - PAD_OFF - 60, cy - 90, cx + PAD_OFF + 60, cy + 90) if rot == 0 else \
            box(cx - 90, cy - PAD_OFF - 60, cx + 90, cy + PAD_OFF + 60)
        if MODULE.intersects(body):
            return 'module'
        if not inner.covers(body):
            return 'inner'
        for j in tree.query(body.buffer(6)):
            if is_pad_or_hole[int(j)] and body.distance(obs[int(j)]) < 6:
                return 'body_pads:' + lbl[int(j)]
        if any(sh.buffer(8).intersects(m['antenna']) for sh in shapes):
            return 'antenna'
        for k, sh in enumerate(shapes):
            for j in tree.query(sh.buffer(6)):
                if meta[int(j)] & {1, 2} and sh.distance(obs[int(j)]) < 6:
                    return f'pad{k+1}:' + lbl[int(j)]
            if not inner.covers(sh) or sh.buffer(8).intersects(m['antenna']):
                return f'pad{k+1}:edge'
        if list(silk_tree.query(body.buffer(6))):
            return 'silk'
        p1 = (cx - PAD_OFF, cy) if rot == 0 else (cx, cy - PAD_OFF)
        for tgt in sorted(taps, key=lambda t: math.hypot(p1[0] - t[0], p1[1] - t[1])):
            if math.hypot(p1[0] - tgt[0], p1[1] - tgt[1]) > 1500:
                return 'tap:far'
            if path(p1, tgt):
                break
        else:
            return 'tap:none'
        return 'designator' if not _designator_ok(cx, cy) else 'OK'

    def _designator_ok(cx, cy, rot=90):
        own = []
        for sgn in (-1, +1):
            px, py = (cx + sgn * PAD_OFF, cy) if rot == 0 else (cx, cy + sgn * PAD_OFF)
            own.append(box(px - PAD_SIDE / 2, py - PAD_SIDE / 2, px + PAD_SIDE / 2, py + PAD_SIDE / 2))
        for ddx, ddy in ((-160, 0), (160, 0), (0, -PAD_OFF - 90), (0, PAD_OFF + 90)):
            dbox = box(cx + ddx - 70, cy + ddy - 30, cx + ddx + 70, cy + ddy + 30)
            if any(dbox.distance(p) < 6 for p in own):
                continue
            if MODULE.intersects(dbox) or not inner.covers(dbox):
                continue
            if list(silk_tree.query(dbox.buffer(6))):
                continue
            ok = True
            for j in tree.query(dbox.buffer(6)):
                if meta[int(j)] & {1, 2} and dbox.distance(obs[int(j)]) < 6:
                    ok = False
                    break
            if ok:
                return True
        return False

    def home(cx, cy, rot=90):
        pads, shapes = [], []
        for sgn in (-1, +1):
            px, py = (cx + sgn * PAD_OFF, cy) if rot == 0 else (cx, cy + sgn * PAD_OFF)
            pads.append((px, py))
            shapes.append(box(px - PAD_SIDE / 2, py - PAD_SIDE / 2,
                              px + PAD_SIDE / 2, py + PAD_SIDE / 2).buffer(6))
        body = box(cx - PAD_OFF - 60, cy - 90, cx + PAD_OFF + 60, cy + 90) if rot == 0 else \
            box(cx - 90, cy - PAD_OFF - 60, cx + 90, cy + PAD_OFF + 60)
        if MODULE.intersects(body) or not inner.covers(body):
            return None
        body_bad = False
        for j in tree.query(body.buffer(6)):
            j = int(j)
            if is_pad_or_hole[j] and body.distance(obs[j]) < 6:
                body_bad = True
                break
        if body_bad or any(sh.buffer(8).intersects(m['antenna']) for sh in shapes):
            return None
        if not all(copper_ok(sh) for sh in shapes):
            return None
        if list(silk_tree.query(body.buffer(6))):
            return None
        p1 = (pads[1][0], pads[1][1])  # pin1 above centre in board coords? pin1 = cy - PAD_OFF
        p1 = (pads[0][0], pads[0][1])
        best = None
        for tgt in sorted(taps, key=lambda t: math.hypot(p1[0] - t[0], p1[1] - t[1])):
            if math.hypot(p1[0] - tgt[0], p1[1] - tgt[1]) > 1500:
                break
            got = path(p1, tgt)
            if got:
                best = got
                break
        if not best:
            return None
        own_pads = [box(px - PAD_SIDE / 2, py - PAD_SIDE / 2, px + PAD_SIDE / 2, py + PAD_SIDE / 2)
                    for px, py in pads]
        for ddx, ddy in ((0, 0), (-160, 0), (160, 0), (0, -PAD_OFF - 90), (0, PAD_OFF + 90),
                         (-160, -PAD_OFF - 90), (160, -PAD_OFF - 90),
                         (-160, PAD_OFF + 90), (160, PAD_OFF + 90)):
            dbox = box(cx + ddx - 70, cy + ddy - 30, cx + ddx + 70, cy + ddy + 30)
            if MODULE.intersects(dbox) or not inner.covers(dbox):
                continue
            if any(dbox.distance(p) < 6 for p in own_pads):
                continue
            if list(silk_tree.query(dbox.buffer(6))):
                continue
            ok = True
            for j in tree.query(dbox.buffer(6)):
                if meta[int(j)] & {1, 2} and dbox.distance(obs[int(j)]) < 6:
                    ok = False
                    break
            if ok:
                return dict(x=cx, y=cy, rotation=rot,
                    pads=[[round(v, 3) for v in pads[0]], [round(v, 3) for v in pads[1]]],
                    deleteStub=STUB_ID, newSegments=best, widthMil=W,
                    designator=[round(cx + ddx, 1), round(cy + ddy, 1)])
        return None
        return None

    cands = []
    for cy in range(1060, 3860, 25):
        for cx in range(-560, 4560, 25):
            for rot in (0, 90):
                got = home(cx, cy, rot)
                if got:
                    got['dist'] = round(math.hypot(cx - 1890, cy - 932.5))
                    cands.append(got)
        if len(cands) >= 400:
            break
    if not cands:
        print('DEBUG failing stages (coarse grid):')
        seen = {}
        for cy in range(1060, 3900, 100):
            row = []
            for cx in range(-500, 4500, 200):
                s = stage(cx, cy, 0)
                tag = s.split(':')[0]
                seen[tag] = seen.get(tag, 0) + 1
                row.append(tag[0] if s == 'OK' else tag.split('_')[0][:4])
            print(cy, ''.join(row))
        print('stage counts:', seen)
    assert cands, 'no legal vertical R5 home'
    best = min(cands, key=lambda c: (c['dist'], len(c['newSegments'])))
    plan = dict(archive=str(ARCHIVE), moduleKeepOutMil=[500, 30, 2460, 1040],
                legalCandidates=len(cands), move=best)
    (R.DATA / 'r5_move_plan.json').write_text(json.dumps(plan, indent=2))
    print(json.dumps(plan, indent=1))


if __name__ == '__main__':
    main()
