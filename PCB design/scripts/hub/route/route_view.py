# Debug view of a routes file around one area: pads (labelled des.pin:net), both layers' tracks (top red, bottom
# blue), vias, the antenna keep-out, the module body and the boost block. Read-only.
# Usage: python route_view.py <routes.json> x0,y0,x1,y1 [out.png] [--scale=1.2] [--layer=0|1]
import json
import os
import sys

from PIL import Image, ImageDraw, ImageFont

import layout_plan as LP
import route_model as M


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    routes = json.load(open(args[0], encoding='utf-8'))
    x0, y0, x1, y1 = (float(v) for v in args[1].split(','))
    out = args[2] if len(args) > 2 else os.path.join(M.HERE, 'data', 'route_view.png')
    s = next((float(a[8:]) for a in sys.argv if a.startswith('--scale=')), 1.2)
    only = next((int(a[8:]) for a in sys.argv if a.startswith('--layer=')), None)
    pads, holes, _ = M.build()
    W, H = int((x1 - x0) * s), int((y1 - y0) * s)
    img = Image.new('RGB', (W, H), (12, 28, 16))
    g = ImageDraw.Draw(img, 'RGBA')
    X = lambda v: (v - x0) * s
    Y = lambda v: (y1 - v) * s
    try:
        font = ImageFont.truetype('arial.ttf', max(9, int(9 * s)))
    except OSError:
        font = ImageFont.load_default()
    for name, rect, col in (('antenna', LP.ANTENNA, (255, 90, 90)), ('body', M.module_body(pads), (255, 220, 80)),
                            ('boost', M.boost_rect(pads), (120, 255, 255))):
        g.rectangle([X(rect[0]), Y(rect[3]), X(rect[2]), Y(rect[1])], outline=col, width=1)
    for h in holes:
        r = max(h['d'], h['ring']) / 2
        g.ellipse([X(h['x'] - r), Y(h['y'] + r), X(h['x'] + r), Y(h['y'] - r)], outline=(220, 220, 220))
    for p in pads:
        g.polygon([(X(a), Y(b)) for a, b in p.poly], fill=(150, 130, 50, 255) if p.net != 'GND' else (110, 110, 110, 255))
    col = {0: (235, 60, 50, 190), 1: (70, 130, 255, 160)}
    for lay in ((1, 0) if only is None else (only,)):
        for net, r in routes.items():
            for (l, ax, ay, bx, by, w) in r['segs']:
                if l == lay:
                    g.line([X(ax), Y(ay), X(bx), Y(by)], fill=col[l], width=max(1, int(w * s)))
    for net, r in routes.items():
        for (vx, vy, d1, d2) in r['vias']:
            rr = d2 * s / 2
            g.ellipse([X(vx) - rr, Y(vy) - rr, X(vx) + rr, Y(vy) + rr], fill=(235, 235, 235))
    # labels: pads (des.pin net), and each net once on its longest segment inside the view
    for p in pads:
        if x0 <= p.cx <= x1 and y0 <= p.cy <= y1:
            g.text((X(p.cx) + 2, Y(p.cy) - 5), '%s.%s %s' % (p.des, p.num, p.net or '-'), fill=(255, 255, 255), font=font)
    for net, r in routes.items():
        best = None
        for (l, ax, ay, bx, by, w) in r['segs']:
            mx, my = (ax + bx) / 2, (ay + by) / 2
            if x0 <= mx <= x1 and y0 <= my <= y1:
                L = abs(bx - ax) + abs(by - ay)
                if best is None or L > best[0]:
                    best = (L, mx, my, l)
        if best and best[0] > 40:
            g.text((X(best[1]) + 2, Y(best[2]) + 2), net, fill=(255, 255, 150) if best[3] == 0 else (170, 210, 255), font=font)
    img.save(out)
    print('wrote', out)


if __name__ == '__main__':
    main()
