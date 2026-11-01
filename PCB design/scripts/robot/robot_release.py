"""Robot saved-board model + fabrication audit + static GND regeneration (from the user's edits).

Reuses the hub's proven geometry helpers (final_ground) but every constant and document id is the
Robot's. Nothing here writes to EasyEDA; the caller serializes the plan into a guarded writer.
"""
import collections
import json
import math
from pathlib import Path

from shapely import affinity
from shapely.geometry import LineString, Point, box
from shapely.ops import unary_union
from shapely.strtree import STRtree
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'hub'))
import final_ground as FG  # hub engine helpers: pad_shape, from_source, pieces, connectivity

HERE = Path(__file__).resolve().parent
DATA = HERE / 'data/release_20261028'
ROBOT = '643ce99542452c8f'
PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864'
MM = 0.0254

# Robot constants (router.py classes / spec §7 + JLC 2-layer 1 oz limits)
NET_CLASS = {'BAT+': 'PWR_HI', 'BAT_SW': 'PWR_HI', '+5V': 'PWR_5V',
             'AMP_D': 'PWR_AMP', 'VAMP_BAT': 'PWR_AMP', 'VAMP': 'PWR_AMP',
             '+3V3': 'PWR_3V3', 'GND': 'GND'}
CLASS_MAIN_MM = {'PWR_HI': 3.0, 'PWR_5V': 2.0, 'PWR_AMP': 1.2, 'PWR_3V3': 0.8, 'SIG': 0.35, 'GND': 0.6}
CLASS_MIN_MM = {'PWR_HI': 1.2, 'PWR_5V': 0.8, 'PWR_AMP': 0.6, 'PWR_3V3': 0.3, 'SIG': 0.25, 'GND': 0.35}
AUX_SIGNAL = {('BAT_SW', 'R24', '1'), ('BAT_SW', 'R26', '1'), ('+5V', 'R27', '1'),
              ('+3V3', 'R14', '1'), ('+3V3', 'R15', '1'), ('+3V3', 'R18', '1'),
              ('+3V3', 'R20', '1'), ('+3V3', 'R22', '1'), ('+3V3', 'J14', '1')}
ANTENNA = box(416, 3640, 1524, 3840)  # layout_plan.ANTENNA; board keeps copper out of the strip
NORMAL, EDGE, MIN_NECK, ANT_MARGIN = 11.0, 16.0, 12.0, 8.0
POWER_SPOKE_PADS = {'J1', 'J2', 'J3', 'J4'}  # XT battery/motor returns: 40 mil spokes, 14 mil gap


def copper_model(archive):
    import gnd_edit
    docs = gnd_edit.read_archive(Path(archive))
    records = docs[ROBOT]['records']
    components = {i: r['body'] for (t, i), r in records.items() if t == 'COMPONENT'}
    attrs = collections.defaultdict(dict)
    for (t, i), r in records.items():
        if t == 'ATTR' and r['body'].get('parentId') in components:
            attrs[r['body']['parentId']][r['body']['key']] = r['body']['value']
    nets = {(json.loads(i)[1], json.loads(i)[3]): r['body']['padNet']
            for (t, i), r in records.items() if t == 'PAD_NET'}
    pads, holes, items = [], [], []
    def add(p, des, rid, cx=0, cy=0, rot=0, net=''):
        center = FG.transform(Point(p['centerX'], p['centerY']), cx, cy, rot)
        angle = rot + p.get('padAngle', 0)
        copper = FG.transform(FG.pad_shape(p), center.x, center.y, angle)
        hd = p.get('hole') or {}
        hole = None
        if hd.get('holeType') == 'ROUND':
            hole = Point(0, 0).buffer(hd['width'] / 2, quad_segs=FG.QS)
        elif hd.get('holeType') == 'SLOT':
            length, width = hd['width'], hd['height']
            hole = LineString([(-(length - width) / 2, 0), ((length - width) / 2, 0)]).buffer(width / 2, quad_segs=FG.QS)
        if hole is not None:
            hole = FG.transform(hole, center.x, center.y, angle + p.get('relativeAngle', 0))
            holes.append(dict(des=des, num=p['num'], shape=hole, net=net))
        layers = (1, 2) if p['layerId'] == 12 else (p['layerId'],)
        pads.append(dict(des=des, num=p['num'], id=rid, net=net, shape=copper, hole=hole,
                         center=(center.x, center.y), layers=layers, native=p))
        items.append(dict(kind='pad', id=rid, net=net, layers=layers, shape=copper, label=f'{des}.{p["num"]}'))
    for cid, c in components.items():
        for (t, rid), r in docs[attrs[cid]['Footprint']]['records'].items():
            if t == 'PAD':
                add(r['body'], attrs[cid]['Designator'], cid + rid, c['x'], c['y'], c['angle'], nets.get((cid, rid), ''))
    for (t, rid), r in records.items():
        b = r['body']
        if t == 'PAD':
            add(b, 'MH', rid, net=b.get('netName', ''))
        elif t == 'LINE' and b.get('layerId') in (1, 2):
            a = (b['startX'], b['startY']); z = (b['endX'], b['endY'])
            g = Point(a) if a == z else LineString([a, z])
            items.append(dict(kind='track', id=rid, net=b['netName'], layers=(b['layerId'],),
                              shape=g.buffer(b['width'] / 2, quad_segs=FG.QS),
                              label=f"{b['netName']} track {rid}", width=b['width'],
                              start=a, end=z))
        elif t == 'VIA':
            xy = (b['centerX'], b['centerY'])
            items.append(dict(kind='via', id=rid, net=b['netName'], layers=(1, 2),
                              shape=Point(xy).buffer(b['viaDiameter'] / 2, quad_segs=FG.QS),
                              label=f"{b['netName']} via {rid}"))
            holes.append(dict(des='via', num=rid, shape=Point(xy).buffer(b['holeDiameter'] / 2, quad_segs=FG.QS), net=b['netName']))
        elif t == 'FILL':
            raise ValueError('Source still contains fills; delete them first')
    outline = next(FG.from_source(r['body']['path']) for (t, _), r in records.items()
                   if t == 'POLY' and r['body'].get('layerId') == 11)
    return dict(docs=docs, records=records, components=components, attrs=attrs, pads=pads,
                holes=holes, items=items, outline=outline, antenna=ANTENNA)


def audit(model):
    m = model
    report = {}
    # 1) track widths per class (allowed = class minimum level; flag long below-main power runs)
    by_class = collections.defaultdict(lambda: collections.Counter())
    below_main = []
    for i in m['items']:
        if i['kind'] != 'track':
            continue
        net = i['net']
        cls = NET_CLASS.get(net, 'SIG')
        width_mm = i['width'] * MM
        by_class[cls][round(width_mm, 3)] += 1
        if width_mm + 1e-6 < CLASS_MAIN_MM[cls]:
            length = math.hypot(i['end'][0] - i['start'][0], i['end'][1] - i['start'][1])
            below_main.append([cls, net, round(width_mm, 3), round(length * MM, 2), i['id']])
    report['trackWidthsByClass'] = {k: dict(v) for k, v in by_class.items()}
    report['belowMainWidths'] = below_main
    # 2) copper pair clearances
    tree = STRtree([i['shape'] for i in m['items']])
    worst = []
    for index, a in enumerate(m['items']):
        for j in tree.query(a['shape'].buffer(0.2 / MM)):
            j = int(j)
            if j <= index:
                continue
            b = m['items'][j]
            if a['net'] == b['net'] and a['net']:
                continue
            if not set(a['layers']) & set(b['layers']):
                continue
            gap = a['shape'].distance(b['shape']) * MM
            if a['shape'].intersection(b['shape']).area > 1e-5:
                raise AssertionError(f'SHORT: {a["label"]} vs {b["label"]}')
            if gap < 0.127:
                raise AssertionError(f'below JLC 0.127mm: {a["label"]} {b["label"]} {gap}')
            worst.append([round(gap, 4), a['label'], b['label']])
    worst.sort()
    report['minCopperGap'] = worst[0] if worst else None
    report['gapsBelowDesignSixMil'] = [r for r in worst if r[0] < 0.1524]
    # 3) hole-to-foreign-copper + hole-to-hole + annular rings
    bad_hole_cu, bad_hh, rings = [], [], []
    for p in m['pads']:
        if p['hole'] is None or not p['native'].get('plated'):
            continue
        assert p['shape'].buffer(0.001).covers(p['hole']), f'hole outside pad {p["des"]}.{p["num"]}'
        rings.append([round(p['shape'].exterior.distance(p['hole'].exterior) * MM, 4), f'{p["des"]}.{p["num"]}'])
    for index, h in enumerate(m['holes']):
        minimum = 0.2 if h['des'] == 'via' else 0.28
        for j in tree.query(h['shape'].buffer(0.4 / MM)):
            i = m['items'][int(j)]
            if h['net'] == i['net'] and h['net']:
                continue
            if f'{h["des"]}.{h["num"]}' == i['label']:
                continue
            gap = h['shape'].distance(i['shape']) * MM
            if gap < minimum - 1e-4:
                bad_hole_cu.append([round(gap, 4), f'{h["des"]}.{h["num"]}', i['label'], minimum])
        for j in STRtree([q['shape'] for q in m['holes']]).query(h['shape'].buffer(0.5 / MM)):
            j = int(j)
            if j <= index:
                continue
            q = m['holes'][j]
            minimum = 0.2 if h['des'] == 'via' and q['des'] == 'via' else 0.45
            gap = h['shape'].distance(q['shape']) * MM
            if gap < minimum - 1e-4:
                bad_hh.append([round(gap, 4), f'{h["des"]}.{h["num"]}', f'{q["des"]}.{q["num"]}', minimum])
    rings.sort()
    report['holeToForeignCopper'] = bad_hole_cu
    report['holeToHole'] = bad_hh
    report['minComponentPthRing'] = rings[0]
    via_rings = sorted((round(v['shape'].exterior.distance(next(q['shape'] for q in m['holes'] if q['num'] == v['id'])) * MM, 4), v['id'])
                        for v in m['items'] if v['kind'] == 'via')
    report['minViaRing'] = via_rings[0]
    # 4) edge
    edge = min(i['shape'].distance(m['outline'].exterior) * MM for i in m['items'])
    assert edge >= 0.2 - 1e-4, edge
    outside = [i['label'] for i in m['items'] if not m['outline'].covers(i['shape'])]
    assert not outside, outside
    report['minEdgeMm'] = round(edge, 4)
    # 5) antenna strip: no power-class or GND-stitching copper inside (thin signals allowed rev B)
    ant_hits = [(i['net'], i['kind'], i['label']) for i in m['items']
                if i['shape'].intersects(ANTENNA) and NET_CLASS.get(i['net'], 'SIG') in {'PWR_HI', 'PWR_5V', 'PWR_AMP', 'PWR_3V3'}]
    report['powerInAntenna'] = ant_hits
    # 6) connectivity of every named net (fills excluded; GND checked again after pour)
    from final_ground import connectivity
    disconnected = {}
    for net in sorted({i['net'] for i in m['items'] if i['net']}):
        sub = dict(m, items=[dict(i, net='GND') for i in m['items'] if i['net'] == net])
        _, bad = connectivity({1: [], 2: []}, sub)
        if bad:
            disconnected[net] = bad
    report['disconnectedNetsBeforePour'] = disconnected
    # 7) XT drills vs approved D-031 sizes
    xt = []
    approved = {'eac10ca20e6778ba': (4.0, None), '9e78c3c3ca74ad42': (2.5, 3.5), '9197bb2e6c38da76': (2.5, 3.5)}
    for fp, (hole_mm, pad_mm) in approved.items():
        for (t, i), r in m['docs'][fp]['records'].items():
            if t != 'PAD':
                continue
            b = r['body']
            xt.append([fp, b['num'], round(b['hole']['width'] * MM, 3), round(b['defaultPad']['width'] * MM, 3),
                       hole_mm, pad_mm])
    report['xtDrillState'] = xt
    return report


def ground_plan(model):
    """Fills from CURRENT geometry: conservative strip AND actual antenna overhang excluded,
    thermal reliefs, island-free via FG.connectivity (real intersects predicate, hub method)."""
    import rf_repair
    m = model
    keepout = unary_union([ANTENNA, rf_repair.antenna_box(m)])
    allowed = m['outline'].buffer(-EDGE).difference(keepout.buffer(ANT_MARGIN))
    fills = {}
    for layer in (1, 2):
        foreign = [i for i in m['items'] if i['net'] != 'GND' and layer in i['layers']]
        blocked = unary_union([i['shape'].buffer(NORMAL, quad_segs=24) for i in foreign])
        area = allowed.difference(blocked)
        for p in m['pads']:
            if p['net'] != 'GND' or layer not in p['layers']:
                continue
            gap = 14 if p['des'] in POWER_SPOKE_PADS else 12
            ring = p['shape'].buffer(gap, quad_segs=24).difference(p['shape'])
            area = area.difference(ring)
            cx, cy = p['center']
            a, b, c, d = p['shape'].bounds
            reach = max(c - a, d - b) / 2 + gap + 6
            sw = 40 if p['des'] in POWER_SPOKE_PADS else 20
            spokes = unary_union([box(cx - sw / 2, cy - reach, cx + sw / 2, cy + reach),
                                  box(cx - reach, cy - sw / 2, cx + reach, cy + sw / 2)])
            area = unary_union([area, spokes.intersection(allowed.difference(blocked))])
        area = area.buffer(-MIN_NECK / 2, quad_segs=16).buffer(MIN_NECK / 2, quad_segs=16).intersection(area)
        from shapely.geometry import Polygon
        fills[layer] = [p for p in (area.geoms if hasattr(area, 'geoms') else [area])
                        if isinstance(p, Polygon) and p.area >= 400]
    connected, disconnected = FG.connectivity(fills, m)
    if disconnected:
        raise AssertionError(f'GND items unreachable even with fills: {disconnected}')
    return {l: connected[l] for l in (1, 2)}


def serialize(fills):
    """Hole-free rounded pieces; precision-rounded self-touches split (hub repair pass)."""
    from shapely.geometry import Polygon
    from shapely.validation import make_valid
    pieces = []
    for layer in (1, 2):
        for poly in fills[layer]:
            for q in FG.pieces(poly.simplify(0.1, preserve_topology=True)):
                coords = [(round(x, 3), round(y, 3)) for x, y in q.exterior.coords]
                if coords[0] != coords[-1]:
                    coords.append(coords[0])
                pieces.append(dict(layer=layer,
                                   source=[*coords[0], 'L', *[v for p in coords[1:] for v in p]]))
    fixed = []
    for p in pieces:
        nums = [n for n in p['source'] if not isinstance(n, str)]
        poly = Polygon(list(zip(nums[::2], nums[1::2])))
        if poly.is_valid:
            fixed.append(p)
            continue
        for q in FG.pieces(make_valid(poly)):
            coords = list(q.exterior.coords)
            src = [*coords[0], 'L', *[n for pt in coords[1:] for n in pt]]
            assert FG.from_source(src).is_valid
            fixed.append(dict(layer=p['layer'], source=src))
    return fixed


def verify_plan(model, pieces):
    """Independent exact-geometry acceptance of the serialized plan (hub verify_plan gates)."""
    import rf_repair
    m = model
    fills = {l: [FG.from_source(p['source']) for p in pieces if p['layer'] == l] for l in (1, 2)}
    bylayer = {l: unary_union(fills[l]) for l in (1, 2)}
    overlaps, bad, minimum = [], [], 1e9
    for i in m['items']:
        if i['net'] == 'GND':
            continue
        for l in i['layers']:
            g = bylayer[l]
            d = g.distance(i['shape'])
            minimum = min(minimum, d)
            if g.intersection(i['shape']).area > 0.001:
                overlaps.append([l, i['label']])
            if d < NORMAL - 0.5:
                bad.append([l, i['label'], round(d, 3), NORMAL])
    connected, disconnected = FG.connectivity(fills, m)
    floating = sum(len(fills[l]) - len(connected[l]) for l in (1, 2))
    assert floating == 0, f'{floating} fill pieces outside the main GND network'
    edge = min(g.distance(m['outline'].exterior) for g in bylayer.values())
    strip = min(g.distance(ANTENNA) for g in bylayer.values())
    overhang = min(g.distance(rf_repair.antenna_box(m)) for g in bylayer.values())
    assert all(m['outline'].covers(g) for g in bylayer.values()), 'Fill outside outline'
    assert edge >= EDGE - 0.2, edge
    assert strip >= ANT_MARGIN - 0.2 and overhang >= ANT_MARGIN - 0.2, (strip, overhang)
    return dict(fillPieces=len(pieces), layerCounts={str(l): len(fills[l]) for l in fills},
                foreignOverlaps=overlaps, clearanceViolations=bad,
                minForeignGapMil=round(minimum, 3), edgeClearanceMil=round(edge, 3),
                antennaStripMarginMil=round(strip, 3), antennaOverhangMarginMil=round(overhang, 3),
                disconnectedGnd=disconnected, floatingFillPieces=floating)


def main():
    DATA.mkdir(parents=True, exist_ok=True)
    archive = HERE.parents[1] / 'Robot/WRO_Robot_user_edited_20261028-1059.epro2'
    m = copper_model(archive)
    report = audit(m)
    (DATA / 'audit_before_fixes.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k != 'belowMainWidths'}, indent=1))
    print('below-main segments:', len(report['belowMainWidths']))


if __name__ == '__main__':
    main()
