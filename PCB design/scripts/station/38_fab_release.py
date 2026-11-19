"""Decode the station manufacturing export, audit the actual CAM package, build the release folder."""
import base64
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'hub'))
sys.path.insert(0, str(HERE.parent / 'robot'))
import repaired_hub as RH
import fab_release as RF  # reuse inspect_gerber

OUT = HERE.parents[1] / 'station/fabrication/20261028-final'
RESPONSE = HERE / 'data/mfg_export_station_20261028.json'


def main():
    v = json.loads(RESPONSE.read_text(encoding='utf-8-sig'))
    if isinstance(v, str):
        v = json.loads(v)
    assert v['drc'] in ([], [[]]) or not v['drc'], f'strict DRC not clean: {v["drc"]}'
    gerber = base64.b64decode(v['gerber']['b64'])
    assert len(gerber) == v['gerber']['size']
    OUT.mkdir(parents=True, exist_ok=True)
    zip_path = OUT / 'STATION_GERBER_20261028_FINAL.zip'
    assert not zip_path.exists()
    zip_path.write_bytes(gerber)
    report = RF.inspect_gerber(zip_path)
    pth = sum(report['drillHitsBySizeMm'].values())
    assert 38 <= pth <= 44, f'PTH drill hits {pth} vs ~41 design holes (40 THT pins + 1 via)'
    assert report['outlineBoundsMm'] and abs(report['outlineBoundsMm']['x'] - 50.8) < 0.15 \
        and abs(report['outlineBoundsMm']['y'] - 31.877) < 0.15, report['outlineBoundsMm']
    # Top = 59 hand-routed tracks (+ pad draws) -> ~74 ops; bottom = 209-fill GND plane.
    assert len(report['copperDrawOps']) == 2 and list(report['copperDrawOps'].values())[0] >= 50 \
        and list(report['copperDrawOps'].values())[1] >= 1000, report['copperDrawOps']
    assert len(report['silkLayers']) == 2, report['silkLayers']
    if v.get('bom'):
        (OUT / 'STATION_RAW_BOM_20261028.csv').write_bytes(base64.b64decode(v['bom']['b64']))
    (OUT / 'MFG-INSPECTION.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    lines = []
    for f in sorted(OUT.iterdir()):
        if f.name != 'SHA256SUMS.txt':
            lines.append(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}')
    (OUT / 'SHA256SUMS.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(json.dumps(dict(drillHits=report['drillHitsBySizeMm'], pthTotal=pth,
                          outline=report['outlineBoundsMm'], copperOps=report['copperDrawOps'],
                          zip=str(zip_path)), indent=2))


if __name__ == '__main__':
    main()
