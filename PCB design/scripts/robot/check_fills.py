# Read-back gate for the static GND fills (D-038). Checks the fills that are actually in PCB1 against the copper that
# is actually in PCB1; EasyEDA's DRC does not see fill overlaps, so run it after every copper edit and every 42_fill.js.
#   - format: every fill is ONE closed 'L' ring on layer 1 (top) / 2 (bottom); with --plan also = data/fill_plan.json
#     (same count, each fill's bbox matches a plan piece);
#   - other-net copper (tracks, vias, pads, plated mounting-hole rings): 0 overlap, exact gap per item (histogram +
#     the closest spots), NPTH holes, board edge, antenna keep-out.
# Read-only. Usage: python check_fills.py data/<copper dump>.json data/<fills dump>.json [--plan] [--min=10]
#   copper dump: see check_eda_copper.py;  fills dump: [[layer, complexPolygonSource], ...] read from PCB1
import json
import os
import sys

from shapely.geometry import LineString, Point, Polygon, box
from shapely.strtree import STRtree

import layout_plan as LP
import route_model as M
import check_eda_copper as CE

HERE = os.path.dirname(os.path.abspath(__file__))
BINS = (6.0, 8.0, 10.0, 11.0, 12.0)


def rings_of(src):
    """complexPolygon source -> list of rings [(x, y), ...]; raises on anything but straight 'L' runs."""
    rings = src if src and isinstance(src[0], list) else [src]
    out = []
    for r in rings:
        pts, i = [(float(r[0]), float(r[1]))], 2
        while i < len(r):
            if r[i] != 'L':
                raise ValueError('unsupported command %r' % (r[i],))
            i += 1
            while i + 1 < len(r) and not isinstance(r[i], str):
                pts.append((float(r[i]), float(r[i + 1])))
                i += 2
        out.append(pts)
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    min_ok = next((float(a[6:]) for a in sys.argv if a.startswith('--min=')), 10.0)
    routes, _ = CE.load(args[0])
    raw = json.load(open(args[1], encoding='utf-8'))
    raw = json.loads(raw) if isinstance(raw, str) else raw
    pads, holes, _ = M.build()
    fills, bad = {0: [], 1: []}, []
    for k, (layer, src) in enumerate(raw):
        rings = rings_of(src)
        if len(rings) != 1 or layer not in (1, 2):
            bad.append((k, layer, len(rings)))
            continue
        pts = rings[0]
        if pts[0] != pts[-1]:
            bad.append((k, 'open ring'))
        g = Polygon(pts)
        if not g.is_valid:
            bad.append((k, 'invalid polygon'))
        fills[0 if layer == 1 else 1].append(g)
    n = len(fills[0]) + len(fills[1])
    print('fills read back: %d (top %d, bottom %d); format problems: %d %s' % (n, len(fills[0]), len(fills[1]), len(bad), bad[:6]))
    if '--plan' in sys.argv:
        # = plan means the same SHAPE: a bbox match alone hid a stale piece after a via moved inside it (D-041)
        plan = json.load(open(os.path.join(HERE, 'data', 'fill_plan.json'), encoding='utf-8'))
        miss, differ = 0, []
        for L in (0, 1):
            pp = [Polygon(p) for p in plan[str(L)]]
            used = [False] * len(pp)
            for g in fills[L]:
                c = [i for i, q in enumerate(pp) if not used[i] and all(abs(a - b) < 1.0 for a, b in zip(g.bounds, q.bounds))]
                if not c:
                    miss += 1
                    continue
                i = min(c, key=lambda i: g.hausdorff_distance(pp[i]))
                used[i] = True
                d = g.hausdorff_distance(pp[i])   # largest deviation; the 2-decimal rounding of the JS is ~0.01 mil
                if d > 0.3:
                    differ.append(('top' if L == 0 else 'bot', tuple(round(v) for v in g.bounds), round(d, 2)))
        print('plan: %d pieces; read-back fills without a plan piece: %d; differing in shape (> 0.3 mil): %d %s' % (
            len(plan['0']) + len(plan['1']), miss, len(differ), differ[:6]))
    # other-net copper per layer: (shape for the index, exact distance function, label, location)
    items = {0: [], 1: []}
    for net, r in routes.items():
        if net == 'GND':
            continue
        for s in r['segs']:
            ln = LineString([(s[1], s[2]), (s[3], s[4])]) if (s[1], s[2]) != (s[3], s[4]) else Point(s[1], s[2])
            items[s[0]].append((ln, s[5] / 2, '%s track' % net, (s[1], s[2], s[3], s[4])))
        for v in r['vias']:
            for L in (0, 1):
                items[L].append((Point(v[0], v[1]), v[3] / 2, '%s via' % net, (v[0], v[1])))
    for p in pads:
        if p.net != 'GND':
            for L in p.layers:
                items[L].append((Polygon(p.poly), 0.0, '%s.%s pad (%s)' % (p.des, p.num, p.net or 'no net'), (p.cx, p.cy)))
    npth = []
    for h in holes:
        for L in (0, 1):
            if h['ring']:
                items[L].append((Point(h['x'], h['y']), h['ring'] / 2, 'hole ring', (h['x'], h['y'])))
        if not h['ring']:
            npth.append((Point(h['x'], h['y']), h['d'] / 2))
    gaps, overlaps = [], []
    for L in (0, 1):
        tree = STRtree([it[0] for it in items[L]])
        for g in fills[L]:
            for j in tree.query(g.buffer(40)):
                shp, r, lab, loc = items[L][int(j)]
                d = g.distance(shp) - r
                gaps.append((round(d, 2), L, lab, loc))
                if d <= 0.0:
                    overlaps.append((L, lab, loc, round(shp.buffer(r, 16).intersection(g).area, 1) if r else round(shp.intersection(g).area, 1)))
    # one value per copper item: its closest fill
    best = {}
    for d, L, lab, loc in gaps:
        k = (L, lab, loc)
        best[k] = min(best.get(k, 1e9), d)
    vals = sorted((d, k) for k, d in best.items())
    print('other-net copper next to a fill (within 40 mil): %d items; overlaps: %d %s' % (len(vals), len(overlaps), overlaps[:6]))
    edges = (0.0,) + BINS + (1e9,)
    hist = ['%s: %d' % ('< %g' % b if a == 0 else ('>= %g' % a if b > 1e8 else '%g-%g' % (a, b)),
                       sum(1 for d, _ in vals if a <= d < b)) for a, b in zip(edges, edges[1:])]
    print('gap fill -> other-net copper (mil), per item:', ', '.join(hist))
    low = [(d, k) for d, k in vals if d < min_ok]
    print('items closer than %g mil: %d' % (min_ok, len(low)))
    for d, (L, lab, loc) in low[:25]:
        print('   %5.2f mil  %s  %-28s at %s' % (d, 'top' if L == 0 else 'bot', lab, tuple(round(v) for v in loc)))
    # holes, edge, antenna
    hd = min((g.distance(c) - r for L in (0, 1) for g in fills[L] for c, r in npth), default=None)
    board = box(0, 0, LP.W, LP.H)
    ed = min(g.distance(board.exterior) if board.contains(g) else -1.0 for L in (0, 1) for g in fills[L])
    a = LP.ANTENNA
    ant = box(a[0], a[1], a[2], LP.H + 50)
    ad = min(g.distance(ant) for L in (0, 1) for g in fills[L])
    print('NPTH holes: min gap %s mil; board edge: min %.2f mil; antenna keep-out: min %.2f mil' % (
        'n/a' if hd is None else '%.2f' % hd, ed, ad))


if __name__ == '__main__':
    main()
