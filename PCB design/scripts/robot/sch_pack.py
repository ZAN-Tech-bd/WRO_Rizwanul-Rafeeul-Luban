# Robot carrier v0.4 (copied from attendance) — offline replica of 13_layout.js's block packing (same rect rule: symbol bbox + every pin end + a STUB-long stub in the
# pin direction + margin M; shelf rows inside a block; blocks stacked in columns) using 12_dump.js output, to search for
# column widths / block orders that fit the A4 frame before moving anything in EasyEDA.
# Usage: python sch_pack.py data/sch_dump_v04.txt   -> prints the best arrangements (heights per column)
import itertools
import json
import re
import sys

STUB, M, GAP, TITLE = 20, 7, 10, 16
DIR = {0: (1, 0), 90: (0, 1), 180: (-1, 0), 270: (0, -1)}
BLOCKS = {
    'A': ['J1', 'J5', 'J2', 'J3', 'J4', 'R26', 'LED1', 'R27', 'LED2', 'R24', 'R25', 'C11'],   # v0.5
    'B': ['U1A', 'U1B', 'J14'],
    'C': ['J6', 'R2', 'R3'],
    'D': ['J7', 'R4', 'R5', 'R6', 'C7', 'C8'],
    'E': ['U2A', 'U2B', 'C3', 'C4', 'R7', 'R8', 'R9', 'U3', 'U4', 'D1', 'D2', 'JP1', 'C9', 'C10'],
    'F': ['J8', 'C5', 'C6', 'R10', 'J9', 'R11', 'R12', 'R13'],
    'G': ['J10', 'R14', 'R15', 'R16', 'R17', 'J11', 'J15', 'R18', 'R19', 'C12', 'J12', 'SW1', 'R20', 'R21', 'C13', 'J13', 'SW2', 'R22',
          'R23', 'C14'],
}


def load(path):
    raw = open(path, encoding='utf-8').read().strip()
    txt = json.loads(raw) if raw.startswith('"') else raw
    rect = {}
    for line in txt.split('\n'):
        if not line.strip():
            continue
        head, pins = line.split('|', 1)
        m = re.match(r'(\w+)@(-?\d+),(-?\d+)\[(\d+)x(\d+)@(-?\d+),(-?\d+)\]', head)
        des, w, h, ox, oy = m.group(1), int(m.group(4)), int(m.group(5)), int(m.group(6)), int(m.group(7))
        x0, x1, y0, y1 = ox, ox + w, oy, oy + h
        for p in pins.split(';'):
            if not p:
                continue
            xy = p.split('@')[1].split(',')
            px, py, rot = int(xy[0]), int(xy[1]), int(xy[2]) % 360
            ux, uy = DIR.get(rot, (-1, 0))
            for qx, qy in ((px, py), (px + ux * STUB, py + uy * STUB)):
                x0, x1, y0, y1 = min(x0, qx), max(x1, qx), min(y0, qy), max(y1, qy)
        rect[des] = (x0 - M, x1 + M, y0 - M, y1 + M)
    return rect


def block_height(rect, parts, width):
    cx, cy, row = 4, -TITLE, 0
    for d in parts:
        x0, x1, y0, y1 = rect[d]
        w, h = x1 - x0, y1 - y0
        if w > width - 8:
            return 1e9
        if cx + w > width - 4 and cx > 4:
            cy -= row
            cx, row = 4, 0
        cx += w
        row = max(row, h)
    return -(cy - row - 4)


def main():
    rect = load(sys.argv[1])
    if '--a3' in sys.argv:            # A3 frame 1654 x 1169, title block bottom-right (~355 x 175, as on A4)
        X0, X1, Y0, Y1 = 25, 1630, 15, 1145
        TB_X, TB_Y = 1295, 175
    else:                             # A4 frame 1170 x 825
        X0, X1, Y0, Y1 = 25, 1145, 15, 800
        TB_X, TB_Y = 815, 175         # title block: x >= 815 and y <= 175 is reserved
    best = []
    names = list(BLOCKS)
    # 3 columns: try split points and every assignment of blocks to columns (order within a column as listed)
    for c1 in range(300, 700, 20):
        for c2 in range(X0 + c1 + 270, X1 - 260, 20):
            cols = [(X0, X0 + c1), (X0 + c1 + 10, c2), (c2 + 10, X1)]
            widths = [b - a for a, b in cols]
            H = {b: [block_height(rect, BLOCKS[b], w) for w in widths] for b in names}
            for assign in itertools.product(range(3), repeat=len(names)):
                tot = [0, 0, 0]
                for b, c in zip(names, assign):
                    tot[c] += H[b][c] + GAP
                # column bottom limit: the last column sits above the title block if it reaches x >= 815
                ok = True
                for ci, (a, bx) in enumerate(cols):
                    floor = TB_Y + 10 if bx > TB_X else Y0
                    if Y1 - tot[ci] + GAP < floor:
                        ok = False
                        break
                if ok:
                    slack = min(Y1 - tot[ci] + GAP - (TB_Y + 10 if cols[ci][1] > TB_X else Y0) for ci in range(3))
                    best.append((slack, c1, c2, assign, [round(t) for t in tot]))
    best.sort(reverse=True)
    print('fitting arrangements:', len(best))
    for b in best[:8]:
        slack, c1, c2, assign, tot = b
        cols = {}
        for n, c in zip(names, assign):
            cols.setdefault(c, []).append(n)
        print('slack %d  split %d/%d  cols %s  heights %s' % (slack, c1, c2, [cols.get(i, []) for i in range(3)], tot))
    if not best:
        need = {b: min(block_height(rect, BLOCKS[b], w) for w in (340, 420, 500)) for b in names}
        print('no A4 fit; block heights at 340/420/500 wide:', need)


if __name__ == '__main__':
    main()
