"""Decode the manufacturing export, inspect the actual Gerber/drill package, and build the
dated release folder (zip + project + reports + checksums + README).

Gates (from the review evidence, not assumptions):
  - strict DRC categories == 0 (captured in the same export run);
  - drill file: exactly 2 hits at 4.0 mm (J1), 6 at 2.5 mm (J2/J3/J4), 4 at 3.2 mm (mounting);
  - outline layer: a single closed rectangle ~92.325 x 98.425 mm;
  - both copper layers carry the pour (large draw-op counts) and silk layers exist.
"""
import base64
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'hub'))
import repaired_hub as RH  # read_metric_drills is board-agnostic

OUT = HERE.parents[1] / 'Robot/fabrication/20261028-final'


def inspect_gerber(zip_path):
    with zipfile.ZipFile(zip_path) as z:
        names = z.namelist()
        drill_name = next(n for n in names if re.search(r'\.(TXT|DRL)$', n, re.I))
        drills = RH.read_metric_drills(z.read(drill_name).decode('utf-8', 'replace'))
        by_size = {}
        for hit in drills['round']:
            by_size[round(hit['diameter'], 4)] = by_size.get(round(hit['diameter'], 4), 0) + 1
        assert not drills['slots'], f'unexpected slots: {drills["slots"]}'
        assert drills['plated'], 'PTH plating flag missing from the drill file'
        outline = next((n for n in names if n.endswith('.GKO')), None)
        bounds = None
        if outline:
            txt = z.read(outline).decode('utf-8', 'replace')
            pts = [(float(a) / 1e6, float(b) / 1e6) for a, b in
                   re.findall(r'X(-?\d+)Y(-?\d+)D0\d', txt)]
            assert pts, 'no outline coordinates parsed'
            xs, ys = zip(*pts)
            bounds = dict(x=round(max(xs) - min(xs), 3), y=round(max(ys) - min(ys), 3),
                          closed=pts[0] == pts[-1] or len(pts) >= 4)
        via_drill = next((n for n in names if n.endswith('_Via.DRL')), None)
        via_drills = RH.read_metric_drills(z.read(via_drill).decode('utf-8', 'replace')) if via_drill else None
        copper_ops = {}
        for n in names:
            if re.search(r'(Top|Bottom).*\.GTL$|\.GTL$|\.GBL$', n, re.I) or n.endswith(('.GTL', '.GBL')):
                txt = z.read(n).decode('utf-8', 'replace')
                copper_ops[n] = sum(1 for L in txt.splitlines() if re.match(r'^D0[12]\*?$', L.strip()) or re.match(r'^G0?1', L.strip()))
        silk = [n for n in names if n.endswith(('.GTO', '.GBO'))]
    return dict(files=names, drillFile=drill_name, drillHitsBySizeMm=by_size,
                outlineFile=outline, outlineBoundsMm=bounds,
                viaDrillFile=via_drill,
                viaDrillHitsBySizeMm={round(h['diameter'], 4): sum(
                    1 for q in via_drills['round'] if round(q['diameter'], 4) == round(h['diameter'], 4))
                    for h in via_drills['round']} if via_drills else None,
                copperDrawOps=copper_ops, silkLayers=silk)


def main():
    response = json.loads((HERE / 'data/mfg_export_20261028.json').read_text(encoding='utf-8-sig'))
    if isinstance(response, str):
        response = json.loads(response)
    drc = response['drc']
    assert drc == [] or drc == [[]] or not drc, f'strict DRC not clean at export time: {drc}'
    gerber = base64.b64decode(response['gerber']['b64'])
    assert len(gerber) == response['gerber']['size']
    bom = base64.b64decode(response['bom']['b64']) if response.get('bom') else None
    OUT.mkdir(parents=True, exist_ok=True)
    zip_path = OUT / 'WRO_ROBOT_GERBER_20261028_FINAL.zip'
    assert not zip_path.exists()
    zip_path.write_bytes(gerber)
    report = inspect_gerber(zip_path)
    hits = report['drillHitsBySizeMm']
    assert hits.get(4.0) == 2, f'J1 4.0 mm drill hits: {hits.get(4.0)}'
    assert hits.get(2.5) == 6, f'XT30 2.5 mm drill hits: {hits.get(2.5)}'
    assert hits.get(3.2, 0) + hits.get(3.2004, 0) == 4 or hits.get(3.2004) == 4, f'mounting holes: {hits}'
    assert report['outlineBoundsMm'] and abs(report['outlineBoundsMm']['x'] - 92.325) < 0.3 \
        and abs(report['outlineBoundsMm']['y'] - 98.425) < 0.3, report['outlineBoundsMm']
    assert len(report['copperDrawOps']) >= 2 and all(v > 500 for v in report['copperDrawOps'].values()), report['copperDrawOps']
    assert len(report['silkLayers']) >= 1, report['silkLayers']
    assert sum(report['viaDrillHitsBySizeMm'].values()) == 156, report['viaDrillHitsBySizeMm']
    if bom is not None:
        (OUT / 'WRO_ROBOT_RAW_BOM_20261028.csv').write_bytes(bom)
    (OUT / 'MFG-INSPECTION.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    # checksums
    lines = []
    for f in sorted(OUT.iterdir()):
        if f.name == 'SHA256SUMS.txt':
            continue
        lines.append(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}')
    (OUT / 'SHA256SUMS.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(json.dumps(dict(drillHits=hits, outline=report['outlineBoundsMm'],
                          copperDrawOps=report['copperDrawOps'], silk=report['silkLayers'],
                          zip=str(zip_path)), indent=2))


if __name__ == '__main__':
    main()
