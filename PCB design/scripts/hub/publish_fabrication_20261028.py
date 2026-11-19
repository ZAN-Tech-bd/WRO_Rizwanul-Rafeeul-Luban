"""Inspect current drill/outline/BOM outputs before packaging a uniquely named review bundle."""
import base64
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import re
import shutil
import zipfile

import repaired_hub as R
from release_audit_20261028 import FINAL,EVIDENCE

OUTPUT=R.HERE.parents[1]/'hub/fabrication/20261028-after-user-edits'


def main():
    audit=json.loads((EVIDENCE/'audit.json').read_text(encoding='utf-8'))
    assert audit['physicalDrcFindings']==0 and audit['currentSchematicPinNetsMatched']==256
    capture=R.json_value(EVIDENCE/'manufacturing_export.json')
    zipbytes=base64.b64decode(capture['gerber']['b64'])
    with zipfile.ZipFile(io.BytesIO(zipbytes)) as z:
        assert z.testzip() is None
        expected={'Gerber_TopLayer.GTL','Gerber_BottomLayer.GBL','Gerber_TopSilkscreenLayer.GTO',
                  'Gerber_BottomSilkscreenLayer.GBO','Gerber_TopSolderMaskLayer.GTS','Gerber_BottomSolderMaskLayer.GBS',
                  'Gerber_BoardOutlineLayer.GKO','Gerber_DocumentLayer.GDL','Drill_PTH_Through.DRL',
                  'Drill_PTH_Through_Via.DRL','How-to-order-PCB.txt'}
        assert set(z.namelist())==expected,'Unexpected fabrication file set'
        outline=z.read('Gerber_BoardOutlineLayer.GKO').decode('utf-8')
        assert '%FSLAX46Y46*%' in outline and '%MOMM*%' in outline
        vertices=[(int(x)/1e6,int(y)/1e6) for x,y in re.findall(r'G01X(-?\d+)Y(-?\d+)D0[12]\*',outline)]
        assert vertices==[(-17.145,0.),(-17.145,100.965),(119.253,100.965),(119.253,0.),(-17.145,0.)],vertices
        drills=R.read_metric_drills(z.read('Drill_PTH_Through.DRL').decode('utf-8'))
        via_drills=R.read_metric_drills(z.read('Drill_PTH_Through_Via.DRL').decode('utf-8'))
        assert drills['plated'] and via_drills['plated']
        assert len(drills['slots'])==6
        model=R.copper_model(FINAL);matched=set();slot_matches=[]
        from shapely import affinity
        expected_slots=[p for p in model['pads'] if p['native'].get('hole',{}).get('holeType')=='SLOT']
        assignment=R.match_slot_geometries(
            [affinity.scale(p['hole'],.0254,.0254,origin=(0,0)) for p in expected_slots],drills['slots'])
        slot_by_pad={p['id']:drills['slots'][ix] for p,ix in zip(expected_slots,assignment)}
        for pad in model['pads']:
            h=pad['native'].get('hole')
            if not h:continue
            cx,cy=(v*.0254 for v in pad['center'])
            diameter=(h['height'] if h['holeType']=='SLOT' else h['width'])*.0254
            if h['holeType']=='SLOT':
                match=slot_by_pad[pad['id']]
                slot_matches.append(dict(pin=pad['num'],net=pad['net'],centerMm=[cx,cy],diameterMm=diameter,
                                         totalLengthMm=math.dist(match['start'],match['end'])+diameter,
                                         fullGeometryOrientationCheck=True))
            else:
                matches=[i for i,q in enumerate(drills['round']) if math.dist((q['x'],q['y']),(cx,cy))<.003 and abs(q['diameter']-diameter)<.003]
                assert len(matches)==1,(pad['des'],pad['num'],'Missing/ambiguous finished drill')
                matched.add(matches[0])
        for (kind,rid),record in model['records'].items():
            if kind!='VIA':continue
            b=record['body'];cx=b['centerX']*.0254;cy=b['centerY']*.0254;diameter=b['holeDiameter']*.0254
            for data in (drills,via_drills):
                matches=[i for i,q in enumerate(data['round']) if math.dist((q['x'],q['y']),(cx,cy))<.003 and abs(q['diameter']-diameter)<.003]
                assert len(matches)==1,('Via drill missing',rid)
                if data is drills:matched.add(matches[0])
        assert len(matched)==len(drills['round']),'Unexpected extra drill coordinates'
        assert len(via_drills['round'])==280
        # Check the corrected narrow Q1 copper aperture is emitted, not only present in native metadata.
        top=z.read('Gerber_TopLayer.GTL').decode('utf-8')
        assert re.search(r'%ADD\d+O,1\.11\d+X1\.6',top) or re.search(r'%ADD\d+O,1\.6\d*X1\.11',top) \
            or '1.114999' in top,'Q1 corrected oblong aperture absent from top copper'
        file_sizes={n:z.getinfo(n).file_size for n in z.namelist()}
    rawbom=base64.b64decode(capture['bom']['b64'])
    text=rawbom.decode('utf-16') if rawbom[:2] in (b'\xff\xfe',b'\xfe\xff') else rawbom.decode('utf-8-sig')
    rows=list(csv.DictReader(io.StringIO(text),delimiter='\t'))
    j3=next(row for row in rows if row.get('Designator')=='J3')
    assert j3['Comment']=='KCD4-203-B' and 'C20088586' not in str(j3)
    assert sum(int(row['Quantity']) for row in rows)==77,'BOM component count mismatch'
    q1=next(row for row in rows if row.get('Designator')=='Q1')
    # Preserve raw BOM; disclose library aliases instead of silently rewriting device/catalog facts.
    report=dict(gerberEntries=file_sizes,outlineMm=[136.398,100.965],outlineVertexCheck=True,
                roundDrills=len(drills['round']),platedSlots=slot_matches,viaFileDrills=len(via_drills['round']),
                noteViaDrills='Main through-drill includes the vias; the standard separate via file duplicates that subset.',
                everyNativeDrillMatched=True,bomPartCount=77,bomJ3=j3,bomQ1=q1,
                assemblyCaveats=['J3 raw footprint text is a legacy VH-6 library alias; all six actual slots verified.',
                                 'Q1 raw BOM still names SS8050; approved assembly uses user 2N2222A. Verify E-B-C pinout.',
                                 'Bare PCB only; no PCBA/CPL release is claimed.'],
                physicalDrcFindings=0,synchronizationFindings=audit['synchronizationFindings'],
                releaseStatus=audit['releaseStatus'])
    OUTPUT.mkdir(parents=True,exist_ok=True)
    name='WRO_HUB_GERBER_20261028_AFTER_USER_EDITS.zip'
    path=OUTPUT/name
    if path.exists() and path.read_bytes()!=zipbytes:raise FileExistsError('Never overwrite a different review package')
    path.write_bytes(zipbytes)
    (OUTPUT/'WRO_HUB_RAW_BOM_20261028.csv').write_text(text,encoding='utf-8-sig')
    shutil.copyfile(FINAL,OUTPUT/'WRO_HUB_PROJECT_20261028.epro2')
    shutil.copyfile(EVIDENCE/'audit.json',OUTPUT/'AUDIT.json')
    (OUTPUT/'MANUFACTURING-INSPECTION.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    manifest={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUTPUT.iterdir() if p.is_file() and p.name!='SHA256SUMS.txt'}
    (OUTPUT/'SHA256SUMS.txt').write_text(''.join(f'{digest}  {name}\n' for name,digest in sorted(manifest.items())),encoding='utf-8')
    print(json.dumps(dict(path=str(path),bytes=path.stat().st_size,roundDrills=report['roundDrills'],slots=6,
                          everyDrillMatched=True,sha256=manifest[name],status=report['releaseStatus']),indent=1))


if __name__=='__main__':main()
