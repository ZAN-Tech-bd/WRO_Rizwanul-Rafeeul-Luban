"""Diff live track/via state vs the user_edited archive after a batch guard abort."""
import json
import subprocess
import sys

import repaired_hub as R

out = subprocess.run(
    ['node', 'tools/easyeda-bridge/eda.mjs', 'exec', '-f', 'PCB design/scripts/hub/71_repair_pcb_snapshot.js'],
    capture_output=True, text=True, timeout=60)
if out.returncode != 0:
    print(out.stdout[-2000:], out.stderr[-2000:])
    sys.exit(1)
live = json.loads(out.stdout)
if isinstance(live, str):
    live = json.loads(live)
live_lines = live.get('lines') or []
live_vias = live.get('vias') or []
if not live_lines and not live_vias:
    print('snapshot keys:', list(live.keys()))
    print(str(live)[:800])
    sys.exit(1)
if 'lines' not in live:
    # 71 snapshot returns components/strings/attributes/nets, not line dumps; build from live pads probe
    print('71 snapshot has no line dump; using full inventory')
    sys.exit(2)
m = R.copper_model(R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_user_edited_20261027-2358.epro2')

arch_lines = {rid: b for (t, rid), r in m['records'].items()
              if t == 'LINE' and r['body'].get('layerId') in (1, 2)
              for b in [r['body']]}
print('live lines:', len(live_lines), 'archive:', len(arch_lines))
live_map = {l['id']: l for l in live_lines}
for rid, b in arch_lines.items():
    l = live_map.get(rid)
    if not l:
        print('MISSING live:', rid, b.get('netName'))
        continue
    same = (abs(l['x1'] - b['startX']) < .01 and abs(l['y1'] - b['startY']) < .01
            and abs(l['x2'] - b['endX']) < .01 and abs(l['y2'] - b['endY']) < .01
            and abs(l['width'] - b['width']) < .11)
    flipped = (abs(l['x2'] - b['startX']) < .01 and abs(l['y2'] - b['startY']) < .01
               and abs(l['x1'] - b['endX']) < .01 and abs(l['y1'] - b['endY']) < .01)
    if not same and not flipped:
        print('DIFF', rid, b.get('netName'), 'arch',
              (b['startX'], b['startY'], b['endX'], b['endY'], b['width']), 'live', (l['x1'], l['y1'], l['x2'], l['y2'], l['width']))
for rid in live_map:
    if rid not in arch_lines:
        print('EXTRA live:', rid, live_map[rid]['net'], (live_map[rid]['x1'], live_map[rid]['y1'], live_map[rid]['x2'], live_map[rid]['y2']))
print('vias live:', len(live_vias), 'archive:', sum(1 for (t, _) in m['records'] if t == 'VIA'))
arch_vias = {rid: b for (t, rid), r in m['records'].items() if t == 'VIA' for b in [r['body']]}
for rid, b in arch_vias.items():
    l = next((v for v in live_vias if v['id'] == rid), None)
    if not l:
        print('MISSING via:', rid)
    elif not (abs(l['x'] - b['centerX']) < .01 and abs(l['y'] - b['centerY']) < .01):
        print('VIA MOVED', rid, (b['centerX'], b['centerY']), (l['x'], l['y']))
