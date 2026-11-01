"""READ-ONLY topology probe for the +3V3 net around the DevKit antenna strip.

Answers, from the verified saved archive only:
  1. the +3V3 network as a graph (items that touch each other), traced from U1A.1/U1A.2;
  2. what the y=3850 spine connects to at (560,3850) and (1765,3850);
  3. exact U1A/U1B socket pad bounds (pin 1 and 2) that constrain a dip below the antenna;
  4. every copper item intersecting the ACTUAL antenna box
     (pin1_y + 75.2 mil = module top edge; module x span from the two socket columns).
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import robot_release as R

ARCHIVE = Path(__file__).resolve().parents[2] / 'Robot/WRO_Robot_xt_verified_20261028-1351.epro2'


def main():
    m = R.copper_model(ARCHIVE)
    items = [i for i in m['items'] if i['net'] == '+3V3']

    # adjacency by shape intersection (union of copper shapes)
    for i in items:
        i['touches'] = sorted(j['id'] if j['kind'] != 'pad' else j['label']
                              for j in items if j is not i and j['shape'].intersects(i['shape']))

    def describe(i):
        if i['kind'] == 'pad':
            return (f"pad {i['label']} @ "
                    f"{tuple(round(v, 1) for v in i['shape'].centroid.coords[0])}")
        if i['kind'] == 'via':
            return (f"via {i['id']} @ "
                    f"{tuple(round(v, 1) for v in i['shape'].centroid.coords[0])}")
        return (f"track {i['id']} L{i['layers'][0]} w{round(i['width']*R.MM,3)}mm "
                f"({round(i['start'][0],1)},{round(i['start'][1],1)})->({round(i['end'][0],1)},{round(i['end'][1],1)})")

    print('=== +3V3 items (layer,width,endpoints,what they touch) ===')
    for i in items:
        print(f"{describe(i)}  touches: {i['touches']}")

    print('\n=== what is near (555..560, 3850) and (1765, 3850) ===')
    from shapely.geometry import Point
    for xy in [(557.5, 3850), (1765, 3850)]:
        near = [(describe(j), round(j['shape'].distance(Point(xy)), 2))
                for j in m['items'] if j['shape'].distance(Point(xy)) < 60]
        print(f'around {xy}:')
        for d, dist in near:
            print(f'   {dist:6.2f} mil  {d}')

    print('\n=== U1A/U1B socket pads pin1/pin2 exact bounds ===')
    for p in m['pads']:
        if p['des'] in ('U1A', 'U1B') and p['num'] in ('1', '2'):
            b = p['shape'].bounds
            print(f"{p['des']}.{p['num']} net={p['net']!r} bounds=({round(b[0],1)},{round(b[1],1)},"
                  f"{round(b[2],1)},{round(b[3],1)}) native w/h={p['native']['defaultPad']['width']}/"
                  f"{p['native']['defaultPad']['height']}")

    print('\n=== actual antenna box ===')
    u1a1 = next(p for p in m['pads'] if p['des'] == 'U1A' and p['num'] == '1')
    u1b1 = next(p for p in m['pads'] if p['des'] == 'U1B' and p['num'] == '1')
    pin1_y = u1a1['center'][1]
    module_top = pin1_y + 75.2  # 1.91 mm drawing offset
    cols = sorted((u1a1['center'][0], u1b1['center'][0]))
    ant = dict(x0=cols[0] - 50, y0=module_top, x1=cols[1] + 50, y1=module_top + 246)  # 6.24 mm antenna
    print(f"pin1_y={pin1_y} module_top={round(module_top,2)} antenna x={ant['x0']}..{ant['x1']} "
          f"y={ant['y0']}..{ant['y1']} (board top = {m['outline'].bounds[3]})")

    from shapely.geometry import box as sbox
    ab = sbox(ant['x0'], ant['y0'], ant['x1'], ant['y1'])
    print('\n=== copper intersecting the ACTUAL antenna box ===')
    for i in m['items']:
        if i['shape'].intersects(ab):
            d = round(i['shape'].intersection(ab).area, 1)
            print(f'   area~{d:8.1f}  {describe(i)}')

    print('\n=== candidate dip band: foreign top-layer copper in y=3780..3845, x=440..1545 ===')
    band = sbox(440, 3780, 1545, 3845)
    for i in m['items']:
        if i['net'] == '+3V3' or 1 not in i['layers']:
            continue
        if i['shape'].buffer(26).intersects(band):
            b = i['shape'].bounds
            print(f"   {i['label']} net={i['net']!r} bounds=({round(b[0],1)},{round(b[1],1)},"
                  f"{round(b[2],1)},{round(b[3],1)})")


if __name__ == '__main__':
    main()
