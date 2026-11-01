# Summarise EasyEDA Pro footprint files (elibz zips from 07b_fp_files.js): pads (number, x, y, hole, size) and the
# extents of the silkscreen / component-shape layers, in mil and mm. Usage: python read_elibz.py data/fp_files.json
import base64
import io
import json
import sys
import zipfile


def nums(seq):
    out = []
    for v in seq:
        if isinstance(v, (int, float)):
            out.append(v)
        elif isinstance(v, list):
            out.extend(nums(v))
    return out


d = json.loads(open(sys.argv[1], encoding='utf-8').read())
if isinstance(d, str):
    d = json.loads(d)
for lcsc, v in d.items():
    if 'b64' not in v:
        print(lcsc, v)
        continue
    z = zipfile.ZipFile(io.BytesIO(base64.b64decode(v['b64'])))
    efoo = [n for n in z.namelist() if n.endswith('.efoo')][0]
    pads, ext = [], {}
    for line in z.read(efoo).decode('utf-8').splitlines():
        try:
            rec = json.loads(line)
        except ValueError:
            continue
        if rec[0] == 'PAD':
            # ["PAD", id, group, net, layer, number, x, y, rot, hole, shape, ...]
            pads.append((rec[5], rec[6], rec[7], rec[9], rec[10]))
        elif rec[0] in ('POLY', 'FILL') and rec[4] in (3, 48):
            xs = nums(rec[6] if rec[0] == 'POLY' else rec[7])
            pts = [x for x in xs if abs(x) < 5000]
            if len(pts) >= 2:
                e = ext.setdefault(rec[4], [1e9, 1e9, -1e9, -1e9])
                for i in range(0, len(pts) - 1, 2):
                    e[0] = min(e[0], pts[i]); e[2] = max(e[2], pts[i])
                    e[1] = min(e[1], pts[i + 1]); e[3] = max(e[3], pts[i + 1])
    print('==', lcsc, v['mpn'], v['fp'])
    for layer, e in ext.items():
        print('   layer %s extents x %.1f..%.1f y %.1f..%.1f mil  -> %.2f x %.2f mm' % (
            {3: 'silk', 48: 'shape'}[layer], e[0], e[2], e[1], e[3], (e[2] - e[0]) * 0.0254, (e[3] - e[1]) * 0.0254))
    for n, x, y, hole, shape in pads:
        print('   pad %-3s (%8.2f, %8.2f) mil  hole %s  pad %s' % (n, x, y, hole, shape))
    if len(pads) >= 2:
        xs = sorted(set(round(p[1], 1) for p in pads))
        ys = sorted(set(round(p[2], 1) for p in pads))
        print('   pad span x %.2f mm, y %.2f mm' % ((xs[-1] - xs[0]) * 0.0254, (ys[-1] - ys[0]) * 0.0254))
