# Footprint geometry library for the offline router: parses data/fp_lib.json (30_fp_library.js output, elibz zips)
# into pads (number, x, y, rotation, layer, hole, copper polygon) and NPTH holes per LCSC id, in the footprint frame
# (mil, Y up, rotation CCW). Shapes become polygons so clearances can be rasterised exactly.
import base64
import io
import json
import math
import os
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))


def _poly_from_path(seq):
    """EasyEDA path array [x0, y0, 'L', x1, y1, ...] (ARC segments approximated by their end points)."""
    pts, i, cur = [], 0, None
    while i < len(seq):
        v = seq[i]
        if isinstance(v, str):
            if v == 'ARC' or v == 'CARC':
                i += 2  # angle, then end point
                pts.append((seq[i], seq[i + 1]))
                i += 2
                continue
            i += 1
            continue
        pts.append((seq[i], seq[i + 1]))
        i += 2
    return pts


def _shape_polygon(shape, x, y, rot):
    """Copper outline (list of (x, y)) of a pad shape centred at (x, y), rotated by rot degrees CCW."""
    kind = shape[0]
    if kind == 'POLY':
        return [(px, py) for px, py in _poly_from_path(shape[1])]  # already absolute in the footprint frame
    w, h = float(shape[1]), float(shape[2])
    if kind in ('RECT',):
        loc = [(-w / 2, -h / 2), (w / 2, -h / 2), (w / 2, h / 2), (-w / 2, h / 2)]
    else:  # ELLIPSE / OVAL / ROUND / REGULAR_POLYGON -> 16-gon / stadium
        loc = []
        if kind == 'OVAL' and abs(w - h) > 0.01:
            r = min(w, h) / 2
            if w >= h:
                c1, c2 = (-(w / 2 - r), 0), (w / 2 - r, 0)
            else:
                c1, c2 = (0, -(h / 2 - r)), (0, h / 2 - r)
            for k in range(9):
                a = math.pi / 2 + math.pi * k / 8 if w >= h else math.pi + math.pi * k / 8
                loc.append((c1[0] + r * math.cos(a), c1[1] + r * math.sin(a)))
            for k in range(9):
                a = -math.pi / 2 + math.pi * k / 8 if w >= h else math.pi * k / 8
                loc.append((c2[0] + r * math.cos(a), c2[1] + r * math.sin(a)))
        else:
            for k in range(16):
                a = 2 * math.pi * k / 16
                loc.append((w / 2 * math.cos(a), h / 2 * math.sin(a)))
    c, s = math.cos(math.radians(rot)), math.sin(math.radians(rot))
    return [(x + px * c - py * s, y + px * s + py * c) for px, py in loc]


def load(path=os.path.join(HERE, 'data', 'fp_lib.json')):
    d = json.load(open(path, encoding='utf-8'))
    lib = {}
    for lcsc, v in d['files'].items():
        z = zipfile.ZipFile(io.BytesIO(base64.b64decode(v['b64'])))
        efoos = [n for n in z.namelist() if n.endswith('.efoo')]
        if not efoos:  # zip without geometry (seen for C13564 / R0603): borrowed below from a same-name footprint
            lib[lcsc] = {'fp': v['fp'], 'des': v['des'], 'pads': None, 'holes': []}
            continue
        efoo = efoos[0]
        pads, holes = [], []
        for line in z.read(efoo).decode('utf-8').splitlines():
            try:
                rec = json.loads(line)
            except ValueError:
                continue
            if rec[0] == 'PAD':
                # ["PAD", id, group, net, layer, number, x, y, rot, hole, shape, ...]
                layer, num, x, y, rot, hole, shape = rec[4], str(rec[5]), rec[6], rec[7], rec[8] or 0, rec[9], rec[10]
                poly = _shape_polygon(shape, x, y, rot)
                hd, slot = 0.0, 0.0
                if isinstance(hole, list) and hole and hole[0] in ('ROUND', 'SLOT'):
                    hd = max(float(hole[1] or 0), float(hole[2] or 0))
                    if hole[0] == 'SLOT' and float(hole[1] or 0) != float(hole[2] or 0):
                        slot = min(float(hole[1] or 0), float(hole[2] or 0))   # slot width; length = hd, along the pad
                pads.append({'num': num, 'x': x, 'y': y, 'rot': rot, 'layer': layer, 'hole': hd, 'slot': slot,
                             'poly': poly, 'shape': shape[0]})
            elif rec[0] == 'HOLE':
                # ["HOLE", id, group, x, y, diameter, ...]
                holes.append({'x': rec[3], 'y': rec[4], 'd': rec[5]})
            elif rec[0] == 'FILL' and rec[4] == 12 and rec[7] and isinstance(rec[7][0], list) and rec[7][0][0] == 'CIRCLE':
                # EasyEDA Pro stores footprint NPTH holes as a circle FILL on the multi layer (BT1 peg, USB-C pegs)
                c = rec[7][0]
                holes.append({'x': c[1], 'y': c[2], 'd': 2 * c[3]})
        lib[lcsc] = {'fp': v['fp'], 'des': v['des'], 'pads': pads, 'holes': holes}
    for lcsc, f in lib.items():
        if f['pads'] is None:
            twin = next((g for g in lib.values() if g['fp'] == f['fp'] and g['pads'] is not None), None)
            if twin is None:
                raise ValueError('no geometry for %s (%s)' % (lcsc, f['fp']))
            f['pads'], f['holes'], f['borrowed'] = twin['pads'], twin['holes'], True
    return lib, d['parts']


if __name__ == '__main__':
    lib, parts = load()
    for lcsc, f in sorted(lib.items(), key=lambda t: t[1]['fp']):
        shapes = sorted(set(p['shape'] for p in f['pads']))
        print('%-10s %-45s pads %2d holes %d %s %s' % (lcsc, f['fp'][:45], len(f['pads']), len(f['holes']), shapes,
                                                       ','.join(f['des'][:6])))
        for h in f['holes']:
            print('            hole', h)
