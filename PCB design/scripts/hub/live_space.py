"""Hub v0.4 — overlap / edge / hole check on the LIVE board (READ-ONLY, offline), for local moves after the J3
direct-mount switch (H-001). Unlike layout_plan.py (the superseded generated floor plan) it starts from what is on the
board now: part bboxes from a 20b inventory, outline polygon and mounting holes from an exported .epro2.

MOVES = {des: (dx, dy)} shifts single parts (mil); OUTLINE overrides the board outline; HOLE_MOVES shifts holes.
Everything not listed stays where the user put it.

Run: python live_space.py <inventory.json> <export.epro2> [--png name] [--js]
"""
import json
import math
import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from inspect_export import documents

HERE = Path(__file__).resolve().parent
DATA = HERE / 'data'
GAP = 20                 # mil between part bboxes
EDGE = 10                # bbox may not cross the outline (DevKit U1A/U1B excepted: antenna over the edge)
HOLE_CLEAR = 40          # mil from the M3 pad ring

# Local moves for the KCD4-203-B (J3) — empty = report the current board only.
MOVES = {}
OUTLINE = None           # [(x, y), ...] closed polygon in mil, or None = keep the live outline
HOLE_MOVES = {}          # pad id -> (dx, dy)


def parse_inventory(path):
    raw = path.read_text(encoding='utf-8')
    data = json.loads(json.loads(raw)) if raw.startswith('"') else json.loads(raw)
    parts = {}
    for line in data['lines']:
        d, fp, xy, rot, layer, box, pads = line.split('|')
        x, y = map(float, xy.split(','))
        w, h, cx, cy = map(float, re.match(r'([\d.]+)x([\d.]+)\(([-\d.]+),([-\d.]+)\)', box).groups())
        pd = {}
        for p in pads.split(';'):
            m = re.match(r'([^:]+):(.*)@([-\d.]+),([-\d.]+)$', p)
            pd[m.group(1)] = (m.group(2), float(m.group(3)), float(m.group(4)))
        parts[d] = dict(x=x, y=y, rot=rot, w=w, h=h, cx=cx, cy=cy, pads=pd)
    return parts


def board(path):
    """Outline polygon (list of points) and mounting holes [(id, x, y, r)] of PCB1 in the export."""
    pcb = next(r for h, r in documents(path) if h.get('docType') == 'PCB')
    outline = None
    for a, b in pcb:
        if a['type'] == 'POLY' and b.get('layerId') == 11:
            nums = [v for v in b['path'] if not isinstance(v, str)]
            outline = list(zip(nums[0::2], nums[1::2]))
    holes = [(a['id'], b['centerX'], b['centerY'], b['defaultPad']['width'] / 2)
             for a, b in pcb if a['type'] == 'PAD']
    return outline, holes


def box(p, dx=0, dy=0):
    x, y = p['x'] + p['cx'] + dx, p['y'] + p['cy'] + dy
    return (x - p['w'] / 2, y - p['h'] / 2, x + p['w'] / 2, y + p['h'] / 2)


def inside(pt, poly):
    x, y = pt
    c = False
    for (x1, y1), (x2, y2) in zip(poly, poly[1:] + poly[:1]):
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            c = not c
    return c


def box_in_poly(b, poly, margin):
    """All four (margin-shrunk) corners inside, and no outline vertex strictly inside the box."""
    x0, y0, x1, y1 = b[0] + margin, b[1] + margin, b[2] - margin, b[3] - margin
    if not all(inside(c, poly) for c in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]):
        return False
    return not any(x0 < vx < x1 and y0 < vy < y1 for vx, vy in poly)


def check(parts, outline, holes):
    probs = []
    boxes = {d: box(p, *MOVES.get(d, (0, 0))) for d, p in parts.items()}
    ds = sorted(boxes)
    for i, a in enumerate(ds):
        A = boxes[a]
        if not a.startswith('U1') and not box_in_poly(A, outline, -EDGE):
            probs.append(f'{a} off-board {[round(v) for v in A]}')
        for hid, hx, hy, r in holes:
            nx, ny = min(max(hx, A[0]), A[2]), min(max(hy, A[1]), A[3])
            if math.hypot(nx - hx, ny - hy) < r + HOLE_CLEAR:
                probs.append(f'{a} on hole {hid} ({hx:.0f},{hy:.0f})')
        for b in ds[i + 1:]:
            B = boxes[b]
            if min(A[2], B[2]) - max(A[0], B[0]) > -GAP and min(A[3], B[3]) - max(A[1], B[1]) > -GAP:
                probs.append(f'overlap {a} / {b}')
    return probs, boxes


def render(name, parts, boxes, outline, holes):
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    x0, y0, x1, y1 = min(xs) - 150, min(ys) - 300, max(xs) + 150, max(ys) + 400
    S, M = 0.2, 20
    img = Image.new('RGB', (int((x1 - x0) * S) + 2 * M, int((y1 - y0) * S) + 2 * M + 20), 'white')
    g = ImageDraw.Draw(img)
    X = lambda x: M + (x - x0) * S
    Y = lambda y: M + (y1 - y) * S
    try:
        f = ImageFont.truetype('arial.ttf', 11)
    except OSError:
        f = ImageFont.load_default()
    g.line([(X(x), Y(y)) for x, y in outline + outline[:1]], fill='black', width=2)
    for hid, hx, hy, r in holes:
        g.ellipse([X(hx - r), Y(hy + r), X(hx + r), Y(hy - r)], outline='black')
    for d, B in boxes.items():
        moved = d in MOVES
        col = (200, 40, 40) if moved else (40, 40, 40)
        g.rectangle([X(B[0]), Y(B[3]), X(B[2]), Y(B[1])], outline=col, width=2 if moved else 1)
        dx, dy = MOVES.get(d, (0, 0))
        for n, (net, px, py) in parts[d]['pads'].items():
            ax, ay = parts[d]['x'] + px + dx, parts[d]['y'] + py + dy
            g.ellipse([X(ax) - 2, Y(ay) - 2, X(ax) + 2, Y(ay) + 2], fill=(200, 140, 0) if n == '1' else (120, 120, 120))
        g.text((X((B[0] + B[2]) / 2) - 8, Y((B[1] + B[3]) / 2) - 6), d, fill=col, font=f)
    w, h = (max(xs) - min(xs)) * 0.0254, (max(ys) - min(ys)) * 0.0254
    g.text((M, img.height - 18), f'{name}: outline {w:.1f} x {h:.1f} mm; red = moved', fill='black', font=f)
    out = DATA / f'live_space_{name}.png'
    img.save(out)
    return out


if __name__ == '__main__':
    inv, exp = Path(sys.argv[1]), Path(sys.argv[2])
    name = sys.argv[sys.argv.index('--png') + 1] if '--png' in sys.argv else 'current'
    parts = parse_inventory(inv)
    outline, holes = board(exp)
    if OUTLINE:
        outline = OUTLINE
    holes = [(i, x + HOLE_MOVES.get(i, (0, 0))[0], y + HOLE_MOVES.get(i, (0, 0))[1], r) for i, x, y, r in holes]
    probs, boxes = check(parts, outline, holes)
    xs, ys = [p[0] for p in outline], [p[1] for p in outline]
    print(f'{len(parts)} parts, outline x {min(xs):.0f}..{max(xs):.0f} y {min(ys):.0f}..{max(ys):.0f} mil '
          f'({(max(xs) - min(xs)) * 0.0254:.1f} x {(max(ys) - min(ys)) * 0.0254:.1f} mm), {len(probs)} problems')
    for p in probs:
        print('  ' + p)
    print(render(name, parts, boxes, outline, holes))
