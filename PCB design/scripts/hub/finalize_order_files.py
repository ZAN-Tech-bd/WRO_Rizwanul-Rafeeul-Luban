"""Finalize: copy order-ready Gerber zip, check final DRC/geometry against the release export."""
import base64
import json
import shutil
import zipfile
import io
from pathlib import Path

import repaired_hub as R

REL = R.HERE.parents[1] / 'hub/WRO_HUB_REPAIRED_r5_ground_20261027-2255.epro2'
ORDER_DIR = R.HERE.parents[2] / 'exports'
ORDER_DIR.mkdir(exist_ok=True)


def main():
    raw = json.loads((R.DATA / 'manufacturing_export.json').read_text())
    v = json.loads(raw) if isinstance(raw, str) else raw
    zip_path = ORDER_DIR / 'WRO_HUB_ORDER_GERBER_20261027.zip'
    zip_path.write_bytes(base64.b64decode(v['gerber']['b64']))
    with zipfile.ZipFile(zip_path) as z:
        assert z.testzip() is None
        names = z.namelist()
    print('ORDER ZIP:', zip_path, zip_path.stat().st_size, 'bytes,', len(names), 'files')
    bom = base64.b64decode(v['bom']['b64'])
    (ORDER_DIR / 'WRO_HUB_ORDER_BOM_20261027.csv').write_bytes(bom)
    print('ORDER BOM:', ORDER_DIR / 'WRO_HUB_ORDER_BOM_20261027.csv')

    snap = R.json_value(R.DATA / 'repaired_final_snapshot.json')
    drc = snap['drc']
    kinds = [list(e.keys())[0] and e.get('name') for e in drc]
    print('final live DRC:', json.dumps(drc)[:300])
    assert all(e.get('name') in {'Netlist Error'} for e in drc), drc

    m = R.copper_model(REL)
    rep = R.check_ground_pieces(m, m['fills'])
    assert not rep['foreignOverlaps'] and not rep['clearanceViolations'] and not rep['disconnectedGnd']
    assert len(m['fills']) == 2362
    rows = R.silk_rows(m, R.json_value(R.DATA / 'repaired_silk_positioned.json'))
    placed = R.place_silk(rows, m)
    moved = [(a['text'], [a['x'], a['y']], [b['x'], b['y']])
             for a, b in zip(rows, placed) if a['x'] != b['x'] or a['y'] != b['y']]
    assert not moved, moved
    from shapely.geometry import box
    ant = box(1475, 3885.196850393701, 2595, 4075)
    assert not [i['label'] for i in m['items'] if i['shape'].intersects(ant)]
    r5 = [p for p in m['pads'] if p['des'] == 'R5']
    mod = box(500, 30, 2460, 1040)
    assert all(not mod.intersects(p['shape']) for p in r5)
    stub_gone = ('LINE', '39b1d74c0d7108f7') not in m['records']
    assert stub_gone
    print('RELEASE VERIFIED: 2362 fills clean, silk in place, antenna clear, R5 outside module, stub gone')
    (R.DATA / 'final_release_report.json').write_text(json.dumps(
        dict(release=str(REL), orderZip=str(zip_path), fills=2362,
             drcSummary=[e.get('name') for e in drc],
             padNetRows=270, r5NotUnderModule=True), indent=2))


if __name__ == '__main__':
    main()
