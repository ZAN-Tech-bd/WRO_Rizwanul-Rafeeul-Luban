# Rescue fill pockets: for every fill region (either layer) that holds GND pads / vias but is not in the main GND
# network, find a stitching-via site inside it where the OTHER layer belongs to the main network (both shrunk by the
# via radius + margin), away from SMD pads, drills and the antenna keep-out. Repeats until nothing more joins.
# Writes <dump>_stitch.json (dump + the new vias) and prints the vias to add. Read-only towards EasyEDA.
# Usage: python pocket_stitch.py data/<dump>.json
import json
import math
import sys

from shapely.geometry import MultiPolygon, Point, Polygon, box
from shapely.ops import unary_union
from shapely.strtree import STRtree

import check_eda_copper as CE
import layout_plan as LP
import pour_fill as PFL
import route_model as M

VIA = (15.75, 31.5)        # 0.4 / 0.8 mm GND via (spec §7.2)
R = VIA[1] / 2
DRILL_GAP = 12.0           # hole to hole >= 0.3 mm


def fill_regions(routes, pads, holes):
    """Per layer: fill polygons before island removal (same rules as pour_fill.py, thermals skipped)."""
    out = {}
    for L in (0, 1):
        board = box(PFL.EDGE, PFL.EDGE, LP.W - PFL.EDGE, LP.H - PFL.EDGE)
        a = LP.ANTENNA
        board = board.difference(box(a[0] - PFL.ANT_MARGIN, a[1] - PFL.ANT_MARGIN, a[2] + PFL.ANT_MARGIN, LP.H + 50))
        foreign, gnd = [], []
        for net, r in routes.items():
            for s in r['segs']:
                if s[0] == L:
                    (gnd if net == 'GND' else foreign).append(PFL.seg_shape(s))
            for v in r['vias']:
                (gnd if net == 'GND' else foreign).append(Point(v[0], v[1]).buffer(v[3] / 2, quad_segs=PFL.QS))
        for p in pads:
            if L in p.layers:
                (gnd if p.net == 'GND' else foreign).append(Polygon(p.poly))
        for h in holes:
            foreign.append(Point(h['x'], h['y']).buffer(max(h['ring'], h['d']) / 2 + (0 if h['ring'] else PFL.HOLE_CLR - PFL.CLR),
                                                       quad_segs=PFL.QS))
        area = board.difference(unary_union(foreign).buffer(PFL.CLR, quad_segs=PFL.QS))
        area = area.buffer(-PFL.MIN_W / 2, quad_segs=PFL.QS).buffer(PFL.MIN_W / 2, quad_segs=PFL.QS)
        area = unary_union([area, unary_union(gnd).intersection(area.buffer(1))])
        out[L] = [g for g in (area.geoms if isinstance(area, MultiPolygon) else [area]) if g.area >= PFL.MIN_AREA]
    return out


def network(regs, pads, routes):
    nodes = [(L, g, 'fill') for L in (0, 1) for g in regs[L]]
    for p in pads:
        if p.net == 'GND':
            nodes.append(('both' if p.tht else 0, Polygon(p.poly), 'pad:%s.%s' % (p.des, p.num)))
    for v in routes['GND']['vias']:
        nodes.append(('both', Point(v[0], v[1]).buffer(v[3] / 2, quad_segs=PFL.QS), 'via'))
    for s in routes['GND']['segs']:
        nodes.append((s[0], PFL.seg_shape(s), 'seg'))
    par = list(range(len(nodes)))

    def f(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i
    tree = STRtree([n[1] for n in nodes])
    for i, (la, sa, _) in enumerate(nodes):
        for j in tree.query(sa, predicate='intersects'):
            j = int(j)
            if j > i and (la == 'both' or nodes[j][0] == 'both' or la == nodes[j][0]):
                par[f(i)] = f(j)
    comps = {}
    for i in range(len(nodes)):
        comps.setdefault(f(i), []).append(i)
    main = max(comps.values(), key=lambda c: sum(1 for i in c if nodes[i][2].startswith('pad')))
    return nodes, comps, set(main)


def main():
    routes, _ = CE.load(sys.argv[1])
    pads, holes, _ = M.build()
    smd = unary_union([Polygon(p.poly) for p in pads if not p.tht])
    drills = [(v[0], v[1], v[2] / 2) for r in routes.values() for v in r['vias']]
    drills += [(p.cx, p.cy, p.hole / 2) for p in pads if p.tht and p.hole]
    drills += [(h['x'], h['y'], h['d'] / 2) for h in holes]
    added = []
    for rnd in range(6):
        regs = fill_regions(routes, pads, holes)
        nodes, comps, main = network(regs, pads, routes)
        lonely = sorted(nodes[i][2][4:] for c in comps.values() if not set(c) <= main and not (set(c) & main)
                        for i in c if nodes[i][2].startswith('pad'))
        print('round %d: GND pads outside the main network: %s' % (rnd, lonely))
        if not lonely:
            break
        main_fill = {L: unary_union([nodes[i][1] for i in main if nodes[i][0] == L and nodes[i][2] == 'fill']) for L in (0, 1)}
        progress = False
        for c in comps.values():
            if set(c) & main or not any(nodes[i][2].startswith('pad') or nodes[i][2] == 'via' for i in c):
                continue
            for L in (0, 1):
                pocket = unary_union([nodes[i][1] for i in c if nodes[i][0] == L and nodes[i][2] == 'fill'])
                if pocket.is_empty:
                    continue
                site = pocket.buffer(-(R + 1)).intersection(main_fill[1 - L].buffer(-(R + 1))).difference(smd.buffer(R + 4))
                if site.is_empty:
                    continue
                cands = [site.representative_point()] + [Point(q) for g in getattr(site, 'geoms', [site])
                                                         for q in list(getattr(g, 'exterior', g).coords)[::3]]
                best = None
                for q in cands:
                    if not site.buffer(0.01).contains(q):
                        continue
                    if any(math.hypot(q.x - x, q.y - y) - rr - VIA[0] / 2 < DRILL_GAP for x, y, rr in drills):
                        continue
                    best = q
                    break
                if best:
                    v = (round(best.x, 1), round(best.y, 1), VIA[0], VIA[1])
                    routes['GND']['vias'].append(v)
                    drills.append((v[0], v[1], VIA[0] / 2))
                    added.append(v)
                    print('   via %s joins a pocket (layer %d) to the main %s fill' % (v[:2], L, 'bottom' if L == 0 else 'top'))
                    progress = True
                    break
        if not progress:
            print('   no more sites')
            break
    d = json.load(open(sys.argv[1], encoding='utf-8'))
    d = json.loads(d) if isinstance(d, str) else d
    for v in added:
        d['GND']['vias'].append([v[0], v[1], v[2], v[3], 'stitch'])
    out = sys.argv[1].replace('.json', '_stitch.json')
    json.dump(d, open(out, 'w', encoding='utf-8'))
    print('added %d vias -> %s' % (len(added), out))


if __name__ == '__main__':
    main()
