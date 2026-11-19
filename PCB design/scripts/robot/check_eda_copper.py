# Exact check of the copper that is actually in PCB1 (a dump of EasyEDA's tracks + vias per net, e.g. after manual
# edits), with route_check.check() plus a real connectivity analysis: per net, copper islands (tracks, vias and pads
# joined where they overlap); every non-GND net must be one island; GND pads need a via (or a via-bearing island).
# Also: JLC's hole-edge rules, and no via under the parts in layout_plan.NO_VIA_BODIES (e.g. a USB-C shell, D-047).
# Usage: python check_eda_copper.py data/_eda_dump.json   (dump: {net: {segs: [[layer 1|2, x1, y1, x2, y2, w, id]],
#        vias: [[x, y, hole, dia, id]]}})
import json
import math
import sys
from collections import defaultdict

import layout_plan as LP
import route_check as RC
import route_model as M


def arc_points(x1, y1, x2, y2, angle, step_deg=6.0):
    """Points along an EasyEDA track arc (mil, Y up): start -> end, signed sweep `angle`, positive = CCW."""
    ang = math.radians(angle)
    c = math.hypot(x2 - x1, y2 - y1)
    if c < 1e-9 or abs(math.sin(ang / 2)) < 1e-9:
        return [(x1, y1), (x2, y2)]
    r = c / (2 * math.sin(abs(ang) / 2))
    mx, my = (x1 + x2) / 2.0, (y1 + y2) / 2.0
    h = r * math.cos(ang / 2)
    cx, cy = mx - h * (-(y2 - y1) / c), my - h * ((x2 - x1) / c)
    n = max(2, int(math.degrees(abs(ang)) / step_deg) + 1)
    return [(cx + (x1 - cx) * math.cos(ang * i / n) - (y1 - cy) * math.sin(ang * i / n),
             cy + (x1 - cx) * math.sin(ang * i / n) + (y1 - cy) * math.cos(ang * i / n))
            for i in range(n + 1)]


def _corner_vertex(x1, y1, x2, y2, segs):
    """Intersection V of the two same-net straight tracks that join the arc's endpoints (a fillet bulges toward V).
    segs = the net's segments as (layer, x1, y1, x2, y2, w); returns None when the corner can't be reconstructed."""
    def touching(px, py):
        out = []
        for s in segs:
            for (ax, ay), (bx, by) in (((s[1], s[2]), (s[3], s[4])), ((s[3], s[4]), (s[1], s[2]))):
                if math.hypot(ax - px, ay - py) < 0.05 and math.hypot(bx - px, by - py) > 0.05:
                    out.append((ax, ay, bx, by))
        return out
    a, b = touching(x1, y1), touching(x2, y2)
    for (ax, ay, adx, ady) in a:
        for (bx, by, bdx, bdy) in b:
            d = (adx - ax) * (bdy - by) - (ady - ay) * (bdx - bx)
            if abs(d) < 1e-9:
                continue
            t = ((bx - ax) * (bdy - by) - (by - ay) * (bdx - bx)) / d
            vx, vy = ax + t * (adx - ax), ay + t * (ady - ay)
            if -50 < t < 500 and math.hypot(vx - (x1 + x2) / 2, vy - (y1 + y2) / 2) < 200:
                return vx, vy
    return None


def arc_polyline(a, segs):
    """The arc as a chord polyline, disambiguated to the side its corner vertex lies on. EasyEDA's signed
    ArcAngle alone can't be trusted to pick the bulge side (endpoint-symmetric), so when the joining tracks
    give a vertex V, the candidate whose midpoint is nearer V wins; without V both candidates are kept
    (a small conservative union) so clearances and pours can never overlap the true arc."""
    p = arc_points(a[1], a[2], a[3], a[4], a[5])
    q = arc_points(a[1], a[2], a[3], a[4], -a[5])
    v = _corner_vertex(a[1], a[2], a[3], a[4], segs)
    if v is not None:
        mid = lambda pts: pts[len(pts) // 2]
        dp = math.hypot(mid(p)[0] - v[0], mid(p)[1] - v[1])
        dq = math.hypot(mid(q)[0] - v[0], mid(q)[1] - v[1])
        if abs(dp - dq) > 1e-6:
            return p if dp < dq else q
    return p + q[1:] if p[0] != q[0] else p  # both sides: consecutive polylines through the same endpoints


def load(path):
    d = json.load(open(path, encoding='utf-8'))
    d = json.loads(d) if isinstance(d, str) else d
    routes = {}
    for net, r in d.items():
        if not net:
            continue
        segs = [(0 if s[0] == 1 else 1, s[1], s[2], s[3], s[4], s[5]) for s in r['segs']]
        for a in r.get('arcs', []):   # rounded corners (user edits 2026-10-17): flatten to chords like any track
            pts = arc_polyline(a, r['segs'])
            segs += [(0 if a[0] == 1 else 1, pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1], a[6])
                     for i in range(len(pts) - 1)]
        routes[net] = {'segs': segs, 'vias': [(v[0], v[1], v[2], v[3]) for v in r['vias']]}
    return routes, d.get('', {'segs': [], 'vias': []})


def islands(routes, pads):
    """Per net: union-find over its tracks, vias and pads (overlap on a shared layer = joined)."""
    out = {}
    by_net = defaultdict(list)
    for p in pads:
        if p.net:
            by_net[p.net].append(p)
    for net, r in routes.items():
        items = []  # (kind, layers, geom, halfwidth, label)
        for s in r['segs']:
            items.append(('seg', (s[0],), (s[1], s[2], s[3], s[4]), s[5] / 2, 'seg'))
        for v in r['vias']:
            items.append(('via', (0, 1), (v[0], v[1], v[0], v[1]), v[3] / 2, 'via'))
        for p in by_net.get(net, []):
            items.append(('pad', p.layers, p.poly, 0.0, '%s.%s' % (p.des, p.num)))
        par = list(range(len(items)))

        def f(i):
            while par[i] != i:
                par[i] = par[par[i]]
                i = par[i]
            return i
        for i in range(len(items)):
            for j in range(i + 1, len(items)):
                a, b = items[i], items[j]
                if not set(a[1]) & set(b[1]):
                    continue
                if a[0] == 'pad' and b[0] == 'pad':
                    continue
                if a[0] == 'pad' or b[0] == 'pad':
                    pad, o = (a, b) if a[0] == 'pad' else (b, a)
                    d = RC.seg_poly_dist(o[2], pad[2]) - o[3]
                else:
                    d = RC.seg_seg(a[2], b[2]) - a[3] - b[3]
                if d <= 0.05:
                    par[f(i)] = f(j)
        groups = defaultdict(list)
        for i, it in enumerate(items):
            groups[f(i)].append(it)
        out[net] = list(groups.values())
    return out


JLC_HOLE = {'pth': 11.0, 'via': 7.9, 'npth': 7.9}   # hole edge -> other-net copper, mil (JLC 1-2 layer page,
# 2026-10-15: PTH to track 0.28 mm (0.35 recommended), via hole to track 0.2 mm, NPTH to track 0.2 mm)


def hole_rules(routes, pads, holes):
    """JLC's hole-to-copper minimums: [(gap, kind, hole label, copper label, x, y)] below JLC_HOLE, worst first.
    Holes go through both layers, so all tracks count. Fills are not needed here: they keep CLR from every non-GND
    pad's copper, so hole -> fill >= ring + CLR. Slotted holes (USB-C shell tabs) are stadiums along the pad's long
    axis."""
    from shapely.geometry import LineString, Point, Polygon
    from shapely.strtree import STRtree

    def pad_hole(p):
        if not p.slot:
            return Point(p.cx, p.cy), p.hole / 2
        c = list(Polygon(p.poly).minimum_rotated_rectangle.exterior.coords)
        e = max(((c[1][0] - c[0][0], c[1][1] - c[0][1]), (c[2][0] - c[1][0], c[2][1] - c[1][1])),
                key=lambda v: math.hypot(*v))
        ux, uy = e[0] / math.hypot(*e), e[1] / math.hypot(*e)
        k = (p.hole - p.slot) / 2
        return LineString([(p.cx - ux * k, p.cy - uy * k), (p.cx + ux * k, p.cy + uy * k)]), p.slot / 2
    cu = []   # (geom, radius, net key, label)
    for net, r in routes.items():
        for s in r['segs']:
            g = LineString([(s[1], s[2]), (s[3], s[4])]) if (s[1], s[2]) != (s[3], s[4]) else Point(s[1], s[2])
            cu.append((g, s[5] / 2, net, net + ' track'))
        for v in r['vias']:
            cu.append((Point(v[0], v[1]), v[3] / 2, net, net + ' via'))
    for p in pads:
        lab = '%s.%s' % (p.des, p.num)
        cu.append((Polygon(p.poly), 0.0, p.net or ('nc', lab), lab))
    hs = [('via', Point(v[0], v[1]), v[2] / 2, net, net + ' via') for net, r in routes.items() for v in r['vias']]
    for p in pads:
        if p.tht and p.hole:
            lab = '%s.%s' % (p.des, p.num)
            g, r = pad_hole(p)
            hs.append(('pth', g, r, p.net or ('nc', lab), lab))
    for h in holes:
        hs.append(('pth' if h['ring'] else 'npth', Point(h['x'], h['y']), h['d'] / 2, ('nc', h['name']), h['name']))
    tree = STRtree([c[0] for c in cu])
    out = []
    for kind, pt, r, net, lab in hs:
        lim = JLC_HOLE[kind]
        for j in tree.query(pt.buffer(r + lim + 60)):
            g, cr, cnet, clab = cu[int(j)]
            if cnet == net:
                continue
            d = pt.distance(g) - r - cr
            if d < lim:
                out.append((round(d, 1), kind, lab, clab, round(pt.x), round(pt.y)))
    return sorted(out)


def vias_under_bodies(routes):
    """Vias of any net under the body of a part listed in layout_plan.NO_VIA_BODIES (D-047): a USB-C shell is GND metal
    lying on the solder mask, and a tented via's ring is the weak spot of that mask. Each body is a rectangle in the
    footprint frame (the component-shape outline); every via is rotated into the part's frame (PLAN x, y, rot CCW),
    so the test is exact at any rotation. Returns (part, net, x, y) per offending via."""
    out = []
    for des, (fx0, fy0, fx1, fy1) in getattr(LP, 'NO_VIA_BODIES', {}).items():
        px, py, pr = LP.PLAN[des]
        for net, r_ in routes.items():
            for v in r_['vias']:
                fx, fy = M.rot(v[0] - px, v[1] - py, -pr)
                if fx0 <= fx <= fx1 and fy0 <= fy <= fy1:
                    out.append((des, net, round(v[0], 1), round(v[1], 1)))
    return out


if __name__ == '__main__':
    routes, nonet = load(sys.argv[1])
    print('copper without a net: %d tracks, %d vias' % (len(nonet['segs']), len(nonet['vias'])))
    viol, edge_v, hole_v, zone_v, unc, vip = RC.check(routes, 6.0)
    print('clearance < 6 mil:', len(viol))
    for v in sorted(viol)[:30]:
        print('   ', v)
    print('edge:', len(edge_v), edge_v[:6])
    print('holes:', len(hole_v), hole_v[:6])
    print('antenna zone:', len(zone_v), zone_v[:6])
    print('pads not touched by their net:', len(unc), unc[:30])
    print('via in/at SMD pad:', len(vip), vip[:12])
    print('hole to hole:', len(RC.check.hole2hole), RC.check.hole2hole[:8])
    print('boost block:', len(RC.check.boost), RC.check.boost[:8])
    print('module body:', len(RC.check.body), RC.check.body[:8])
    print('thin power (< 0.35 mm away from pads):', len(RC.check.thin_power), RC.check.thin_power[:16])
    pads, holes, _ = M.build()
    hr = hole_rules(routes, pads, holes)
    print('JLC hole rules (PTH %g, via %g, NPTH %g mil hole edge -> other-net copper):' % (
        JLC_HOLE['pth'], JLC_HOLE['via'], JLC_HOLE['npth']), len(hr), hr[:12])
    ub = vias_under_bodies(routes)
    print('vias under a no-via part body (layout_plan.NO_VIA_BODIES, D-047):', len(ub), ub[:8])
    isl = islands(routes, pads)
    print('connectivity (copper islands per net; GND relies on the pours):')
    for net, groups in sorted(isl.items()):
        if len(groups) > 1 and net != 'GND':
            desc = [sorted(it[4] for it in g if it[0] == 'pad') or ['(%d copper only)' % len(g)] for g in groups]
            print('   %-10s %d islands: %s' % (net, len(groups), desc))
    # GND: pads (SMD) whose island has no via -> rely on the top pour alone
    if 'GND' in isl:
        nov = []
        for g in isl['GND']:
            if not any(it[0] == 'via' for it in g):
                nov += [it[4] for it in g if it[0] == 'pad' and it[1] == (0,)]
        print('GND SMD pads on an island without a via (top pour only):', len(nov), sorted(nov))
