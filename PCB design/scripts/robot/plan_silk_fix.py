"""Read-only planner for the 11 silk artifacts found by 24_check_silk on the final board.

Emits a guarded writer (59_fix_silk.template.js): moves STRING labels and the SW1 designator
to collision-checked spots. Legal = inside outline, >=8 mil from any pad copper, clear of the
4 NPTH holes, not overlapping other silk text boxes (approx metrics), not the same spot twice.
"""
import json
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import robot_release as R

FINAL = HERE.parents[1] / 'Robot/WRO_Robot_silk_round2_20261028-1619.epro2'
OFFENDERS = {'+', 'OBSTACLE 1', 'FC-51 3.3V', 'SDA', '470uF', 'GRN=5V OK'}


def main():
    import gnd_edit
    m = R.copper_model(HERE.parents[1] / 'Robot/WRO_Robot_via_deleted_20261028-1438.epro2')
    docs = gnd_edit.read_archive(FINAL)
    recs = docs[R.ROBOT]['records']
    strings, designators, sw1_pos = [], {}, None
    for (t, i), r in recs.items():
        b = r['body']
        if t == 'STRING' and b.get('layerId') == 3:
            strings.append(dict(id=i, text=b['text'], x=b['x'], y=b['y'], size=b['fontSize']))
        elif t == 'ATTR' and b.get('key') == 'Designator':
            designators[b['parentId']] = dict(id=i, x=b['x'], y=b['y'])
    for cid, c in m['components'].items():
        des = m['attrs'][cid].get('Designator')
        if cid in designators:
            designators[cid]['des'] = des
        if des == 'SW1':
            sw1_pos = (c['x'], c['y'])
    outline = m['outline']
    pads = [p for p in m['pads']]
    holes = [h for h in m['holes'] if h['des'] == 'MH']

    def text_box(x, y, text, size):
        w = len(str(text)) * size * 0.95      # generous: real EasyEDA boxes ran wider than 0.78
        return dict(x0=x - w / 2, y0=y - size * 0.7, x1=x + w / 2, y1=y + size * 0.7)

    obstacles = [(s['x'], s['y'], s['text'], s['size']) for s in strings]
    obstacles += [(v['x'], v['y'], v.get('des', 'XX'), 32) for v in designators.values() if v.get('des')]

    def legal(x, y, text, size, ignore_id):
        tb = text_box(x, y, text, size)
        if not (outline.bounds[0] + 8 <= tb['x0'] and tb['x1'] <= outline.bounds[2] - 8 and
                outline.bounds[1] + 8 <= tb['y0'] and tb['y1'] <= outline.bounds[3] - 8):
            return False
        for p in pads:
            b = p['shape'].bounds
            if not (tb['x1'] + 12 < b[0] or tb['x0'] - 12 > b[2] or tb['y1'] + 12 < b[1] or tb['y0'] - 12 > b[3]):
                return False
        for h in holes:
            hx, hy = h['shape'].centroid.x, h['shape'].centroid.y
            if math.hypot(hx - x, hy - y) < 63 + 20:
                return False
        for ox, oy, otext, osize in obstacles:
            oid = next((s['id'] for s in strings if s['x'] == ox and s['y'] == oy), None)
            if oid == ignore_id:
                continue
            ob = text_box(ox, oy, otext, osize)
            if not (tb['x1'] + 4 < ob['x0'] or tb['x0'] - 4 > ob['x1'] or tb['y1'] + 4 < ob['y0'] or tb['y0'] - 4 > ob['y1']):
                return False
        return True

    plan = {'strings': [], 'designators': []}
    # 1) free STRING labels that 24 flagged (text match; skip duplicates handled by uniqueness of pos)
    seen = set()
    for s in strings:
        if s['text'] not in OFFENDERS:
            continue
        key = (s['text'], round(s['x']), round(s['y']))
        if key in seen:
            continue
        seen.add(key)
        if legal(s['x'], s['y'], s['text'], s['size'], s['id']):
            continue
        found = None
        for dist in range(20, 520, 10):
            for ang in range(0, 360, 15):
                nx, ny = s['x'] + dist * math.cos(math.radians(ang)), s['y'] + dist * math.sin(math.radians(ang))
                if legal(nx, ny, s['text'], s['size'], s['id']):
                    found = (round(nx, 1), round(ny, 1))
                    break
            if found:
                break
        if not found:
            raise SystemExit(f'no legal spot for {s["text"]} @ {s["x"]},{s["y"]}')
        plan['strings'].append(dict(id=s['id'], text=s['text'], before=[s['x'], s['y']], after=list(found)))
    # 2) SW1 designator (off-board): park it inside, near the part, legal
    d = next(v for v in designators.values() if v.get('des') == 'SW1')
    if not legal(d['x'], d['y'], 'SW1', 32, d['id']):
        found = None
        for dist in range(10, 400, 10):
            for ang in range(0, 360, 10):
                nx, ny = sw1_pos[0] + dist * math.cos(math.radians(ang)), sw1_pos[1] + dist * math.sin(math.radians(ang))
                if legal(nx, ny, 'SW1', 32, d['id']):
                    found = (round(nx, 1), round(ny, 1))
                    break
            if found:
                break
        if not found:
            raise SystemExit('no legal spot for SW1 designator')
        plan['designators'].append(dict(id=d['id'], des='SW1', before=[d['x'], d['y']], after=list(found)))
    out = HERE / 'data/release_20261028/silk_fix_plan.json'
    assert not out.exists()
    out.write_text(json.dumps(plan, indent=2), encoding='utf-8')
    source = (HERE / '59_fix_silk.template.js').read_text(encoding='utf-8')
    (HERE / 'data/59_fix_silk.js').write_text(
        source.replace('__PLAN__', json.dumps(plan, separators=(',', ':'))), encoding='utf-8')
    print(json.dumps(plan, indent=2))


if __name__ == '__main__':
    main()
