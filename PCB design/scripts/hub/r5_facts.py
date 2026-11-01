"""Print exact SERVO_IN geometry and the blocking pads around the R5 corridor."""
import json

import repaired_hub as R

m = R.copper_model(R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2')
print('SERVO_IN items:')
for i in m['items']:
    if i['net'] != 'SERVO_IN':
        continue
    if i['kind'] == 'pad':
        print('  pad', i['label'], tuple(round(v, 1) for v in i['shape'].bounds))
        continue
    key = ('LINE' if i['kind'] == 'track' else 'VIA', i['id'])
    b = m['records'].get(key, {}).get('body', {})
    print(' ', i['kind'], i['id'], 'layer', b.get('layerId'), 'w', b.get('width'),
          (b.get('startX'), b.get('startY'), b.get('endX'), b.get('endY')) or
          (b.get('centerX'), b.get('centerY')) or i['shape'].bounds)
print('\nBlockers near corridor x1500-2000 y800-1500:')
for p in m['pads']:
    if p['des'] in ('R5', 'MH'):
        continue
    x0, y0, x1, y1 = p['shape'].bounds
    if 1450 <= x0 <= 2050 and 800 <= y0 <= 1500:
        print('  pad', p['des'], p['num'], p['net'], tuple(round(v) for v in p['center']),
              tuple(round(v) for v in (x0, y0, x1, y1)))
for h in m['holes']:
    b = h['shape'].bounds
    if 1450 <= b[0] <= 2050 and 800 <= b[1] <= 1500 and h['des'] not in ('R5.1', 'R5.2'):
        print('  hole', h['des'], h['num'], tuple(round(v) for v in b))
for i in m['items']:
    if i['kind'] == 'via' and 1450 <= i['shape'].bounds[0] <= 2050 and 800 <= i['shape'].bounds[1] <= 1500:
        print('  via', i['label'], tuple(round(v) for v in i['shape'].bounds))
