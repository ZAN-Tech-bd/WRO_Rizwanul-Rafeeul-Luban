"""H-003: only the 37 off-board station parts; all other live design state is frozen.

Two rows of vertical XH-8s snake S1..S4 across the upper row and S5..S8 back across
the lower row. Short LED hops below each row; zone pull-ups in column gaps;
MQ capacitors at the actual ADC socket pins, under the DevKit (low parts only).
"""
import copy
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from live_space import parse_inventory, board, box, box_in_poly

HERE = Path(__file__).resolve().parent
DATA = HERE / 'data/station_place_20261027'
BACKUP = HERE.parents[1] / 'hub/WRO_HUB_before_station_place_guard_20261027-0148.epro2'
TARGETS = {
    'J10': (2900,3150,90), 'J11': (3420,3150,90), 'J12': (3940,3150,90), 'J13': (4460,3150,90),
    'J14': (4460,1850,90), 'J15': (3940,1850,90), 'J16': (3420,1850,90), 'J17': (2900,1850,90),
    'C3': (1750,3510,0), 'C4': (1750,3375,0), 'C5': (1750,3240,0), 'C6': (1750,3105,0),
    'C7': (2320,3510,180), 'C8': (2320,3375,180), 'C9': (1750,2310,0),
    'R31': (2200,3010,180), 'R32': (2200,2860,180), 'R33': (2200,2710,180), 'R34': (2200,2560,180),
    'R35': (2860,2380,180), 'R36': (3380,2380,180), 'R37': (3900,2380,180),
    'R41': (3160,3160,270), 'R42': (4200,3160,270), 'R43': (4200,1860,270), 'R44': (3160,1860,270),
    'R51': (2200,3210,0), 'R52': (2200,3700,0), 'R53': (2200,2410,180), 'R54': (2200,2260,0),
    'R61': (3160,2530,0), 'R62': (3680,2530,0), 'R63': (4200,2530,0), 'R64': (4630,2440,270),
    'R65': (4200,1250,180), 'R66': (3680,1250,180), 'R67': (3160,1250,180),
}


def source():
    parts = parse_inventory(DATA / 'before_inventory.json')
    outline, holes = board(BACKUP)
    return parts, outline, holes


def transform(p, pose):
    q = copy.deepcopy(p)
    x, y, angle = pose
    delta = math.radians(angle - float(p['rot']))
    c, s = round(math.cos(delta)), round(math.sin(delta))
    rot = lambda x, y: (c*x-s*y, s*x+c*y)
    q['x'], q['y'], q['rot'] = x, y, str(angle)
    q['cx'], q['cy'] = rot(p['cx'], p['cy'])
    q['w'], q['h'] = abs(c)*p['w']+abs(s)*p['h'], abs(s)*p['w']+abs(c)*p['h']
    q['pads'] = {n: (net, *rot(dx,dy)) for n,(net,dx,dy) in p['pads'].items()}
    return q


def placed(parts):
    result = copy.deepcopy(parts)
    for d, pose in TARGETS.items():
        if parts[d]['x'] <= 4695:
            raise ValueError(d + ' is not unplaced; freeze it and re-plan')
        result[d] = transform(parts[d], pose)
    return result


def net_pad(p, net):
    _, dx, dy = next(q for q in p['pads'].values() if q[0] == net)
    return p['x']+dx, p['y']+dy


def check(parts, outline, holes):
    result, problems = placed(parts), []
    boxes = {d:box(p) for d,p in result.items()}
    for d in TARGETS:
        A = boxes[d]
        if not box_in_poly(A, outline, -10):
            problems.append(f'{d} off board: {A}')
        for hid,hx,hy,r in holes:
            nx, ny = min(max(hx,A[0]),A[2]), min(max(hy,A[1]),A[3])
            if math.hypot(nx-hx,ny-hy) < r+40:
                problems.append(f'{d} near hole {hid}')
        for other,B in boxes.items():
            if other == d or (other in TARGETS and other < d):
                continue
            if min(A[2],B[2])-max(A[0],B[0]) > -20 and min(A[3],B[3])-max(A[1],B[1]) > -20:
                problems.append(f'{d} overlaps {other}')
    return problems


def render(parts, outline, holes):
    result = placed(parts)
    scale, margin = .23, 40
    x0,y0,x1,y1 = -675,0,4695,3975
    img = Image.new('RGB',(int((x1-x0)*scale)+2*margin,int((y1-y0)*scale)+2*margin),'#fafafa')
    draw = ImageDraw.Draw(img)
    xy = lambda x,y: (margin+(x-x0)*scale,margin+(y1-y)*scale)
    font = ImageFont.truetype('arial.ttf',12)
    small = ImageFont.truetype('arial.ttf',10)
    draw.polygon([xy(x,y) for x,y in outline],outline='black')
    # Board outline of the mounted MCU, and its in-board antenna/USB approach.
    draw.rectangle([xy(1460,3910),xy(2610,1630)],fill='#edf0f4',outline='#9faaba')
    for hid,hx,hy,r in holes:
        draw.ellipse([xy(hx-r,hy+r),xy(hx+r,hy-r)],outline='black')
    for d,p in result.items():
        if p['x'] > 5000 or p['y'] > 4300:
            continue
        A=box(p)
        color = '#0d7a5c' if d in TARGETS else '#69717b'
        draw.rectangle([xy(A[0],A[3]),xy(A[2],A[1])],outline=color,width=2)
        draw.text(xy(p['x']-40,p['y']+15),d,fill=color,font=small)
        for net,dx,dy in p['pads'].values():
            x,y=xy(p['x']+dx,p['y']+dy)
            draw.ellipse([x-2,y-2,x+2,y+2],fill=color)
    for k in range(1,8):
        p,r,n = result[f'J{k+9}'],result[f'R{60+k}'],result[f'J{k+10}']
        pts=[net_pad(p,f'S{k}_DOUT'),net_pad(r,f'S{k}_DOUT'),net_pad(r,f'S{k+1}_DIN'),net_pad(n,f'S{k+1}_DIN')]
        draw.line([xy(*p) for p in pts],fill='#d47414',width=2)
    draw.text((margin,12),'Station-only placement: green = moved; gray = frozen; orange = LED routing guides (NOT copper)',fill='black',font=font)
    out=DATA/'placement_preview.png'
    img.save(out)
    return out


if __name__ == '__main__':
    parts,outline,holes=source()
    problems=check(parts,outline,holes)
    (DATA/'placement_plan.json').write_text(json.dumps({'targets':TARGETS,'problems':problems},indent=2))
    print('Targets:',len(TARGETS),'Problems:',problems)
    print('Preview:',render(parts,outline,holes))
    raise SystemExit(bool(problems))
