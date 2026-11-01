"""Focused checks: BAT_SW below-class branches, +3V3 antenna escape extent, XT pad geometry."""
import json
import math
from pathlib import Path
from shapely.geometry import Point

import robot_release as RR


def main():
    archive = Path(__file__).resolve().parents[2] / 'Robot/WRO_Robot_user_edited_20261028-1059.epro2'
    m = RR.copper_model(archive)
    pads = {(p['des'], p['num']): p for p in m['pads']}

    print('--- BAT_SW segments below 3.0 mm ---')
    for i in m['items']:
        if i['kind'] != 'track' or i['net'] != 'BAT_SW' or i['width'] * RR.MM >= 3.0 - 1e-9:
            continue
        length = math.hypot(i['end'][0] - i['start'][0], i['end'][1] - i['start'][1])
        near = []
        for (des, num), p in pads.items():
            for pt in (i['start'], i['end']):
                if math.hypot(p['center'][0] - pt[0], p['center'][1] - pt[1]) < 120:
                    near.append(f'{des}.{num}')
        print(f"  {i['width']*RR.MM:.2f}mm x {length*RR.MM:.2f}mm  {i['start']}->{i['end']}  near {sorted(set(near))}")

    print('--- +5V below 2.0 mm ---')
    for i in m['items']:
        if i['kind'] != 'track' or i['net'] != '+5V' or i['width'] * RR.MM >= 2.0 - 1e-9:
            continue
        length = math.hypot(i['end'][0] - i['start'][0], i['end'][1] - i['start'][1])
        print(f"  {i['width']*RR.MM:.2f}mm x {length*RR.MM:.2f}mm")

    print('--- +3V3 inside antenna strip ---')
    zone = RR.ANTENNA
    esc_pads = [pads[('U1A', '1')], pads[('U1A', '2')]]
    total_inside = 0.0
    for i in m['items']:
        if i['net'] != '+3V3' or i['kind'] != 'track':
            continue
        clipped = i['shape'].intersection(zone)
        if clipped.is_empty:
            continue
        length = math.hypot(i['end'][0] - i['start'][0], i['end'][1] - i['start'][1])
        # distance of every point of the inside-part from the escape pads
        d = min(clipped.distance(p['shape']) for p in esc_pads)
        total_inside += clipped.length
        print(f"  insideLen {clipped.length*RR.MM:.2f}mm (seg {length*RR.MM:.1f}mm, w {i['width']*RR.MM:.1f}) "
              f"min-dist-to-U1A.1/2 {d:.0f}mil  {i['id']}")
    print(f'  total +3V3 copper length inside strip: {total_inside*RR.MM:.2f} mm')
    u1a1 = pads[('U1A', '1')]
    print('  U1A.1 center', u1a1['center'], 'zone', zone.bounds)

    print('--- GND stitch vias near antenna strip ---')
    for i in m['items']:
        if i['kind'] == 'via' and i['net'] == 'GND' and i['shape'].distance(zone) < 40:
            print('  ', i['label'], i['shape'].centroid.coords[0], 'dist', round(i['shape'].distance(zone), 1))

    print('--- XT component pad centres (current) ---')
    for des in ('J1', 'J2', 'J3', 'J4'):
        for num in ('1', '2'):
            p = pads[(des, num)]
            print(f'  {des}.{num} net={p["net"]} center={p["center"]} pad={p["native"]["defaultPad"]} hole={p["native"].get("hole")}')


if __name__ == '__main__':
    main()
