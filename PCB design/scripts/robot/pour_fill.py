# Robot carrier v0.6 PCB (pcb-pipeline template, attendance rev C engine) — GND copper on both layers, computed OFFLINE (EasyEDA's pour engine can't be
# driven from the API: pcb_PrimitivePour.create / rebuildCopperRegion throw inside pro-api 0.2.53, 2026-10-15).
# Works from the copper that is actually in PCB1 (a dump of tracks + vias per net) and route_model's exact pads/holes:
#   area   = board outline inset EDGE, minus the antenna keep-out (+ ANT_MARGIN), minus CLR around every non-GND
#            track / via / pad (incl. no-net mounting-hole rings), minus HOLE_CLR around NPTH holes;
#   GND    = SMD GND pads, GND tracks and vias joined directly; THT GND pads get thermal reliefs (GAP ring + 4 spokes)
#            so they stay hand-solderable;
#   clean  = opening with MIN_W (no slivers narrower than that), then only fill pieces in the main GND network (fills,
#            GND pads, tracks and vias joined by overlap, both layers through GND vias / THT pads) are kept.
# Writes 42_fill.js (deletes the previous GND fills, then creates the new ones) and a preview PNG; prints GND pads
# that would still be unconnected.
# Usage: python pour_fill.py data/<copper dump>.json [--dry]   (dump format: see check_eda_copper.py; --dry = no JS)
import json
import math
import os
import sys

from shapely import affinity
from shapely.geometry import LineString, MultiPolygon, Point, Polygon, box
from shapely.ops import unary_union

import layout_plan as LP
import route_model as M
import check_eda_copper as CE

HERE = os.path.dirname(os.path.abspath(__file__))
EDGE = 12.0          # copper zone to board outline (DRC 10 mil + 2)
ANT_MARGIN = 5.0     # keep-out + 5 mil (D-032)
CLR = 11.0           # fill to other-net copper (DRC "Copper/Plane Zone" 10 mil + 1)
HOLE_CLR = 12.0      # fill to NPTH hole edge
GAP, SPOKE = 10.0, 15.0   # thermal relief for THT GND pads: gap ring, spoke width
SPOKE_WIDE = {'J1': 40.0, 'J2': 40.0, 'J3': 40.0, 'J4': 40.0}   # robot: XT battery / motor / buck returns (~5 A)
MIN_W = 8.0          # no fill neck narrower than this
QS = 4               # buffer segments per quarter circle (chord error < 0.5 mil at r = 25 mil, inside CLR's margin)
MIN_AREA = 400.0     # drop fill fragments smaller than 20 x 20 mil


def seg_shape(s):
    (x1, y1, x2, y2, w) = s[1], s[2], s[3], s[4], s[5]
    if abs(x1 - x2) < 1e-6 and abs(y1 - y2) < 1e-6:
        return Point(x1, y1).buffer(w / 2, quad_segs=QS)
    return LineString([(x1, y1), (x2, y2)]).buffer(w / 2, quad_segs=QS)


def main():
    routes, _ = CE.load(sys.argv[1])
    pads, holes, problems = M.build()
    assert not problems, problems[:5]
    board = box(EDGE, EDGE, LP.W - EDGE, LP.H - EDGE)
    a = LP.ANTENNA
    board = board.difference(box(a[0] - ANT_MARGIN, a[1] - ANT_MARGIN, a[2] + ANT_MARGIN, LP.H + 50))
    for h in holes:   # NPTH holes (no ring); plated mounting holes are handled as no-net pads below
        if not h['ring']:
            board = board.difference(Point(h['x'], h['y']).buffer(h['d'] / 2 + HOLE_CLR, quad_segs=QS))
    out = {}
    gnd_items = {0: [], 1: []}     # (kind, shape, label)
    for L in (0, 1):
        foreign, gnd = [], []
        for net, r in routes.items():
            for s in r['segs']:
                if s[0] == L:
                    (gnd if net == 'GND' else foreign).append(seg_shape(s))
            for v in r['vias']:
                c = Point(v[0], v[1]).buffer(v[3] / 2, quad_segs=QS)
                (gnd if net == 'GND' else foreign).append(c)
        thermal = []
        for p in pads:
            if L not in p.layers:
                continue
            shp = Polygon(p.poly)
            if p.net == 'GND':
                gnd.append(shp)
                if p.tht:
                    thermal.append(p)
            else:
                foreign.append(shp)
        for h in holes:   # plated mounting holes without a net: copper ring = foreign
            if h['ring']:
                # 16 segments per quarter: with QS a 6 mm M3 ring's chords cut 2.3 mil off the fill gap (WRO station)
                foreign.append(Point(h['x'], h['y']).buffer(h['ring'] / 2, quad_segs=16))
        keep_out = unary_union(foreign).buffer(CLR, quad_segs=QS)
        area = board.difference(keep_out)
        # thermal reliefs: cut a GAP ring around each THT GND pad, then add 4 spokes back
        for p in thermal:
            shp = Polygon(p.poly)
            ring = shp.buffer(GAP, quad_segs=QS).difference(shp)
            area = area.difference(ring)
            reach = max(p.w, p.h) / 2 + GAP + 4
            sw = SPOKE_WIDE.get(p.des, SPOKE)
            spokes = unary_union([box(p.cx - sw / 2, p.cy - reach, p.cx + sw / 2, p.cy + reach),
                                  box(p.cx - reach, p.cy - sw / 2, p.cx + reach, p.cy + sw / 2)])
            area = unary_union([area, spokes.intersection(board.difference(keep_out))])
        # no slivers: morphological opening (a subset of the area), then join the GND copper directly
        opened = area.buffer(-MIN_W / 2, quad_segs=QS).buffer(MIN_W / 2, quad_segs=QS).intersection(area)
        direct = unary_union([g for g in gnd])
        fill = unary_union([opened, direct.intersection(area)])
        polys = [g for g in (fill.geoms if isinstance(fill, MultiPolygon) else [fill]) if g.area >= MIN_AREA]
        out[L] = polys
        gnd_items[L] = gnd
    # connectivity: fills + GND pads / vias / tracks, joined by overlap; vias and THT pads join the layers
    nodes = []   # (layer or 'both', shape, kind, label)
    for L in (0, 1):
        for i, g in enumerate(out[L]):
            nodes.append((L, g, 'fill', 'fill%d.%d' % (L, i)))
    for p in pads:
        if p.net == 'GND':
            nodes.append(('both' if p.tht else 0, Polygon(p.poly), 'pad', '%s.%s' % (p.des, p.num)))
    for v in routes.get('GND', {'vias': []})['vias']:
        nodes.append(('both', Point(v[0], v[1]).buffer(v[3] / 2, quad_segs=QS), 'via', 'via'))
    for s in routes.get('GND', {'segs': []})['segs']:
        nodes.append((s[0], seg_shape(s), 'seg', 'seg'))
    par = list(range(len(nodes)))

    def f(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i
    from shapely.strtree import STRtree
    tree = STRtree([n[1] for n in nodes])
    for i, (la, sa, ka, _) in enumerate(nodes):
        for j in tree.query(sa, predicate='intersects'):
            j = int(j)
            if j <= i:
                continue
            lb = nodes[j][0]
            if la == 'both' or lb == 'both' or la == lb:
                par[f(i)] = f(j)
    comp = {}
    for i in range(len(nodes)):
        comp.setdefault(f(i), []).append(i)
    main_c = max(comp.values(), key=lambda c: sum(1 for i in c if nodes[i][2] == 'pad'))
    main_set = set(main_c)
    kept = {0: [], 1: []}
    dropped = 0
    for i, (L, g, k, lab) in enumerate(nodes):
        if k == 'fill':
            if i in main_set:
                kept[L].append(g)
            else:
                dropped += 1
    lonely = sorted(nodes[i][3] for c in comp.values() if c is not main_c for i in c if nodes[i][2] == 'pad')
    print('fill pieces: top %d, bottom %d kept; %d islands dropped' % (len(kept[0]), len(kept[1]), dropped))
    print('fill area: top %.0f, bottom %.0f sq mil (board %.0f)' % (
        sum(g.area for g in kept[0]), sum(g.area for g in kept[1]), LP.W * LP.H))
    print('GND pads NOT connected to the main GND network:', len(lonely), lonely)
    if '--dry' in sys.argv:
        return lonely
    # ---- 42_fill.js. EasyEDA Fill primitives keep only the OUTER ring of a polygon (holes are dropped silently and
    # its DRC doesn't see the resulting overlaps, 2026-10-15), so every polygon is cut into hole-free pieces first.
    data = []
    nv = 0
    pieces = {0: [], 1: []}
    for L in (0, 1):
        for g in kept[L]:
            pieces[L] += hole_free(g.simplify(0.4, preserve_topology=True))
    for L in (0, 1):
        for g in pieces[L]:
            rings = [list(g.exterior.coords)]
            src = []
            for ring in rings:
                pts = [(round(x, 2), round(y, 2)) for x, y in ring]
                if pts[0] != pts[-1]:
                    pts.append(pts[0])
                arr = [pts[0][0], pts[0][1], 'L']
                for x, y in pts[1:]:
                    arr += [x, y]
                src.append(arr)
                nv += len(pts)
            data.append([1 if L == 0 else 2, [round(v, 1) for v in g.bounds], src])
    js = FILL_JS.replace('__EDA_PROJECT__', TARGET_PROJECT).replace('/*FILLS*/', json.dumps(data, separators=(',', ':')))
    open(os.path.join(HERE, '42_fill.js'), 'w', encoding='utf-8').write(js)
    print('wrote 42_fill.js: %d hole-free fills (from %d polygons), %d vertices, %.0f kB' % (
        len(data), len(kept[0]) + len(kept[1]), nv, len(js) / 1024))
    json.dump({str(L): [list(g.exterior.coords) for g in pieces[L]] for L in (0, 1)},
              open(os.path.join(HERE, 'data', 'fill_plan.json'), 'w'))
    render(kept, pads, os.path.join(HERE, 'data', 'fill_preview.png'))


def hole_free(poly, ov=1.0):
    """Cut a polygon with holes into hole-free pieces: vertical strips whose borders pass through every hole; strips
    overlap by `ov` mil (inside the polygon) so neighbouring pieces stay joined. Recurses while a piece keeps a hole."""
    if not poly.interiors:
        return [poly]
    xs = sorted(set(round(Polygon(r).representative_point().x, 3) for r in poly.interiors))
    x0, y0, x1, y1 = poly.bounds
    cuts = [x0 - 1] + xs + [x1 + 1]
    out = []
    for a, b in zip(cuts, cuts[1:]):
        part = poly.intersection(box(a - ov, y0 - 1, b + ov, y1 + 1))
        for g in (part.geoms if hasattr(part, 'geoms') else [part]):
            if isinstance(g, Polygon) and g.area >= 1.0:
                out += hole_free(g, ov) if g.interiors else [g]
    return out


def render(kept, pads, path, s=0.3):
    from PIL import Image, ImageDraw
    m = 20
    img = Image.new('RGB', (int(LP.W * s) * 2 + 3 * m, int(LP.H * s) + 2 * m), (10, 10, 10))
    g = ImageDraw.Draw(img, 'RGBA')
    for L in (0, 1):
        ox = m + L * (int(LP.W * s) + m)
        X = lambda v: ox + v * s
        Y = lambda v: m + (LP.H - v) * s
        g.rectangle([X(0), Y(LP.H), X(LP.W), Y(0)], outline=(200, 200, 200))
        col = (200, 60, 50, 255) if L == 0 else (60, 110, 230, 255)
        for poly in kept[L]:
            g.polygon([(X(x), Y(y)) for x, y in poly.exterior.coords], fill=col)
            for r in poly.interiors:
                g.polygon([(X(x), Y(y)) for x, y in r.coords], fill=(10, 10, 10, 255))
        for p in pads:
            if L in p.layers:
                g.polygon([(X(x), Y(y)) for x, y in p.poly], fill=(220, 190, 70, 255) if p.net != 'GND' else (150, 150, 150, 255))
    img.save(path)
    print('preview', path)


TARGET_PROJECT = os.environ.get('EASYEDA_PROJECT', 'WRO_Robot')   # D-048: set to the authorized editing copy when needed

FILL_JS = r"""// GENERATED by pour_fill.py. Attendance carrier v0.3 PCB — GND copper fills (top + bottom), computed offline
// from the routed copper: 11 mil to other nets, 12 mil to the edge, antenna keep-out + 5 mil, thermal reliefs on THT GND
// pads, no slivers < 8 mil, no islands. Idempotent + time-boxed: keeps GND fills whose OUTLINE matches a plan piece
// (same layer, same vertex count, every vertex within 0.3 mil; a bbox match alone kept a stale piece after a via moved
// inside it), deletes other GND fills on layers 1/2, creates the missing ones; re-run until "remaining": 0.
// (EasyEDA's pour engine can't be driven from the API in this build, so these are Fill primitives on net GND.)
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\42_fill.js"
const PROJECT = '__EDA_PROJECT__';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const F = /*FILLS*/;   // [layer, [minx, miny, maxx, maxy], rings]
const t0 = Date.now();
const res = { kept: 0, removed: 0, created: 0, remaining: 0, errors: [] };
const bboxOf = (src) => { const b = [Infinity, Infinity, -Infinity, -Infinity];
  for (const ring of (Array.isArray(src[0]) ? src : [src])) for (let i = 0; i < ring.length; i++) { if (ring[i] === 'L') continue; const x = ring[i], y = ring[i + 1]; i++;
    b[0] = Math.min(b[0], x); b[1] = Math.min(b[1], y); b[2] = Math.max(b[2], x); b[3] = Math.max(b[3], y); } return b; };
const same = (a, b) => a.every((v, i) => Math.abs(v - b[i]) < 1.0);
const pts = (src) => { const out = [];
  for (const ring of (Array.isArray(src[0]) ? src : [src])) for (let i = 0; i < ring.length; i++) { if (typeof ring[i] === 'string') continue; out.push([ring[i], ring[i + 1]]); i++; }
  const a = out[0], z = out[out.length - 1];
  if (out.length > 1 && Math.abs(a[0] - z[0]) < 1e-6 && Math.abs(a[1] - z[1]) < 1e-6) out.pop();
  return out; };
// same outline = every vertex of each ring lies within 0.3 mil of the other ring's edges (two-way). Vertex counts may
// differ: EasyEDA drops redundant (collinear) vertices on create (rev D: 128 planned -> 126 stored), which made an
// equal-count rule re-create that piece on every run.
const segDist = (px, py, ax, ay, bx, by) => { const dx = bx - ax, dy = by - ay, L = dx * dx + dy * dy;
  const t = L ? Math.max(0, Math.min(1, ((px - ax) * dx + (py - ay) * dy) / L)) : 0;
  return Math.hypot(px - ax - t * dx, py - ay - t * dy); };
const nearRing = (pt, R) => R.some((a, i) => { const b = R[(i + 1) % R.length]; return segDist(pt[0], pt[1], a[0], a[1], b[0], b[1]) < 0.3; });
const outlineEq = (A, B) => A.every((p) => nearRing(p, B)) && B.every((p) => nearRing(p, A));
const P = F.map(([, , src]) => pts(src));
const existing = ((await eda.pcb_PrimitiveFill.getAll()) || []).filter((f) => f.getState_Net() === 'GND' && [1, 2].includes(Number(f.getState_Layer())));
const matched = new Array(F.length).fill(false);
const stale = [];
for (const f of existing) {
  let src = null;
  try { src = f.getState_ComplexPolygon().getSource(); } catch (e) {}
  const bb = src ? bboxOf(src) : null;
  const q = src ? pts(src) : null;
  const k = bb ? F.findIndex(([L, b], i) => !matched[i] && L === Number(f.getState_Layer()) && same(b, bb) && outlineEq(P[i], q)) : -1;
  if (k >= 0) { matched[k] = true; res.kept++; } else stale.push(f.getState_PrimitiveId());
}
if (stale.length) { await eda.pcb_PrimitiveFill.delete(stale); res.removed = stale.length; }
for (let i = 0; i < F.length; i++) {
  if (matched[i]) continue;
  if (Date.now() - t0 > 15000) { res.remaining++; continue; }
  const [layer, , src] = F[i];
  try {
    const cp = eda.pcb_MathPolygon.createComplexPolygon(src);
    if (!cp) { res.errors.push('fill ' + i + ': polygon rejected'); continue; }
    const f = await eda.pcb_PrimitiveFill.create(layer, cp, 'GND', 0, 1, false);
    if (f) res.created++; else res.errors.push('fill ' + i + ': create returned nothing');
  } catch (e) { res.errors.push('fill ' + i + ': ' + String(e).slice(0, 80)); }
}
await eda.pcb_Document.save();
return JSON.stringify(res);
"""

if __name__ == '__main__':
    main()
