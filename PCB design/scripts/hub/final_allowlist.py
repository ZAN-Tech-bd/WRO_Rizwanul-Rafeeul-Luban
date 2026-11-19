"""Final offline allowlist: base switch-instance checkpoint vs catalogue-cleared release."""
import collections
import json
import sys

import repaired_hub as R

BASE = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_switch_instance_fixed_20261027-1914.epro2'
FINAL = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_catalogue_cleared_20261027-2040.epro2'
PCB, PAGE = R.TARGET['pcb'], R.TARGET['page']
J3_INSTANCE = 'f26ea52ce64d9e40'


def main():
    base = R.read_archive(BASE)
    final = R.read_archive(FINAL)
    changed = collections.Counter()
    detail = collections.Counter()
    illegal = []
    for doc in base:
        for k in set(base[doc]['records']) | set(final[doc]['records']):
            a = base[doc]['records'].get(k)
            b = final[doc]['records'].get(k)
            if a == b:
                continue
            changed[(doc == PCB, doc == PAGE, k[0])] += 1
            if k[0] in {'FILL', 'ELE_PLACEHOLDER'} and doc == PCB:
                detail['fill_' + ('replaced' if a and b else 'added' if b else 'removed')] += 1
                continue
            if doc == PCB and k[0] == 'LINE':
                detail['rf_line_modified'] += 1
                continue
            if doc == PCB and k[0] in {'STRING', 'ATTR'}:
                detail['silk_style_or_position'] += 1
                continue
            if doc == PAGE and k[0] == 'ATTR' and a and b:
                if a['body'].get('parentId') != J3_INSTANCE:
                    illegal.append([doc == PCB, doc == PAGE, list(k), 'not-j3-owner'])
                    continue
                fields = set(a['body']) | set(b['body'])
                if all(a['body'].get(f) == b['body'].get(f) for f in fields - {'value', 'zIndex'}):
                    detail['j3_schematic_metadata'] += 1
                    continue
            illegal.append([doc == PCB, doc == PAGE, list(k), a is None, b is None])
    report = dict(changeClasses={str(k): v for k, v in changed.items()},
                  allowlistDetail=dict(detail), illegalChanges=illegal[:20])
    print(json.dumps(report, indent=1))
    assert not illegal, illegal[:5]

    recs = final[PCB]['records']
    rf = [(b['startX'], b['startY'], b['endX'], b['endY'])
          for (t, i), r in recs.items() if t == 'LINE'
          for b in [r['body']]
          if b.get('netName') == '+3V3' and b.get('layerId') == 2 and b['startY'] > 3800 and b['endY'] > 3800]
    print('RF final +3V3 segments near top edge:', sorted(rf))
    want = sorted([(2305.0, 3865.0, 2630.0, 3865.0), (2630.0, 3865.0, 2680.0, 3915.0)])
    for seg in want:
        assert seg in [tuple(map(float, s)) for s in rf], seg
    for gone in [(2250.0, 3810.0, 2355.0, 3915.0), (2355.0, 3915.0, 2680.0, 3915.0)]:
        assert gone not in [tuple(map(float, s)) for s in rf], gone

    model = R.copper_model(FINAL)
    assert not R.silk_style_plan(model['records'])
    rows = R.silk_rows(model, R.json_value(R.DATA / 'repaired_silk_positioned.json'))
    assert rows == R.place_silk(rows, model), 'silk placement regressed'
    fills = R.check_ground_pieces(model, model['fills'])
    assert not fills['foreignOverlaps'] and not fills['clearanceViolations'] and not fills['disconnectedGnd']
    print('Saved release: silk in spec, ground plane in spec, strict allowlist PASS')


if __name__ == '__main__':
    main()
