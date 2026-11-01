"""Verify the reviewed GUI J3 metadata ECO, preserve image/copper, and package the standard Gerber.

Does not write to EasyEDA, alter its artwork, or claim raster images are ordinary Gerber geometry.
"""
import base64
import collections
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import zipfile

import repaired_hub as R

BEFORE=R.HERE.parents[1]/'hub/WRO_HUB_REPAIRED_before_sync_fix_with_image_20261028-0222.epro2'
AFTER=R.HERE.parents[1]/'hub/WRO_HUB_REPAIRED_after_sync_fix_with_image_20261028-0225.epro2'
OUTPUT=R.HERE.parents[1]/'hub/fabrication/20261028-sync-fixed'
J3='0cbe9290d583d889'


def main():
    before=R.read_archive(BEFORE);after=R.read_archive(AFTER);pcb=R.TARGET['pcb']
    assert before.keys()==after.keys()
    changed=[]
    for doc in before:
        a=before[doc]['records'];b=after[doc]['records']
        assert a.keys()==b.keys(),f'Record set changed: {doc}'
        changed.extend((doc,k) for k in a if a[k]['raw']!=b[k]['raw'])
    assert changed==[(pcb,('COMPONENT',J3))],changed
    a=before[pcb]['records'][('COMPONENT',J3)]['body'];b=after[pcb]['records'][('COMPONENT',J3)]['body']
    assert {k:v for k,v in a.items() if k!='attrs'}=={k:v for k,v in b.items() if k!='attrs'}
    diff=R.json_value(R.DATA/'sync_diff_with_image_20261028.json')
    allowed=set(re.findall(r'Update Property\tJ3:([^\t\n]+)',diff['dialog']))
    assert len(allowed)==25 and diff['updates']==25
    changed_attrs={k for k in set(a['attrs'])|set(b['attrs']) if a['attrs'].get(k)!=b['attrs'].get(k)}
    assert changed_attrs==allowed,(changed_attrs-allowed,allowed-changed_attrs)
    sch=after[R.TARGET['page']]['records']
    sch_attrs={r['body']['key']:r['body']['value'] for (t,i),r in sch.items()
               if t=='ATTR' and r['body'].get('parentId')=='f26ea52ce64d9e40'}
    assert all(b['attrs'][k]==sch_attrs[k] for k in allowed)
    live=R.json_value(R.DATA/'sync_fixed_pcb_20261028.json')
    erc=R.json_value(R.DATA/'sync_fixed_erc_20261028.json')
    capture=R.json_value(R.DATA/'sync_fixed_manufacturing_with_image_20261028.json')
    assert live['drc']==[] and capture['drc']==[] and erc=={'erc':True,'detailed':[]}
    model=R.copper_model(AFTER);ground=R.check_ground_pieces(model,model['fills'])
    assert not ground['foreignOverlaps'] and not ground['clearanceViolations'] and not ground['disconnectedGnd']
    assert ground['floatingFillPieces']==0 and len(model['fills'])==2331
    pins={p['num']:p['net'] for p in model['pads'] if p['des']=='J3'}
    assert pins=={'1':'BAT+','2':'V12','3':'DC+','4':'BAT-','5':'GND','6':'DC-'}
    slots=[p for p in model['pads'] if p['des']=='J3']
    assert len(slots)==6 and all(p['native']['hole']['holeType']=='SLOT' for p in slots)
    images=[r['body'] for (t,i),r in after[pcb]['records'].items() if t=='OBJ']
    assert len(images)==1 and images[0]['layerId']==3
    zipbytes=base64.b64decode(capture['gerber']['b64'])
    with zipfile.ZipFile(io.BytesIO(zipbytes)) as z:
        assert z.testzip() is None and len(z.namelist())==11
        rounds=R.read_metric_drills(z.read('Drill_PTH_Through.DRL').decode())
        from shapely import affinity
        R.match_slot_geometries([affinity.scale(p['hole'],.0254,.0254,origin=(0,0)) for p in slots],rounds['slots'])
        assert rounds['plated'] and len(rounds['round'])==548 and len(rounds['slots'])==6
        # Prove all actual manufacturing copper/drill/outline/mask output stayed unchanged.
        oldcap=R.json_value(R.DATA/'release_20261028/manufacturing_export.json')
        norm=lambda data:'\n'.join(line for line in data.decode().splitlines()
                                   if not re.match(r'(?:G04 |;)EasyEDA Pro',line))
        physical_layers=['Gerber_TopLayer.GTL','Gerber_BottomLayer.GBL','Gerber_TopSolderMaskLayer.GTS',
                         'Gerber_BottomSolderMaskLayer.GBS','Gerber_BoardOutlineLayer.GKO',
                         'Drill_PTH_Through.DRL','Drill_PTH_Through_Via.DRL']
        with zipfile.ZipFile(io.BytesIO(base64.b64decode(oldcap['gerber']['b64']))) as oldz:
            for name in physical_layers:
                if name.endswith('.DRL'):
                    assert norm(z.read(name))==norm(oldz.read(name)),name
                else:
                    assert R.canonical_gerber_operations(z.read(name).decode())==R.canonical_gerber_operations(oldz.read(name).decode()),name
        top_silk_bytes=z.getinfo('Gerber_TopSilkscreenLayer.GTO').file_size
    probe=R.json_value(R.DATA/'image_only_gerber_probe_20261028.json')
    with zipfile.ZipFile(io.BytesIO(base64.b64decode(probe['b64']))) as z:
        image_only=z.read('Gerber_TopSilkscreenLayer.GTO').decode()
        assert not re.search(r'G0[123]X|D0[123]\*|G36\*',image_only),'Image-only probe unexpectedly contains drawable geometry'
    report=dict(cause='25 stale J3 PCB descriptive attributes differed from the corrected schematic.',
                action='User applied reviewed metadata-only ECO with track-net updating unchecked.',
                changedRecord=dict(document=pcb,type='COMPONENT',id=J3),
                propertyCount=25,properties=sorted(allowed),erc=erc,strictPcbDrc=live['drc'],
                allOtherNativeRecordsByteIdentical=True,allImageObjectsAndResourcesPreserved=True,
                copperDrillOutlineAndMaskGerbersUnchanged=True,j3CustomSlotsPreserved=True,
                j3Nets=pins,ground=ground,topSilkscreenGerberBytes=top_silk_bytes,
                rasterImage=dict(filename=images[0]['fileName'],layer=3,preserved=True,
                                 ordinaryGerberIncludesRaster=False,imageOnlyProbeHasGeometry=False),
                artworkGate='Convert/prepare image for ordinary monochrome fabrication or verify a supported color-silkscreen workflow; no conversion was authorized or performed.',
                hardwareGates=['real switch fit/support/contact action','actual Q1 pinout','full-load voltage-drop/thermal testing'])
    OUTPUT.mkdir(parents=True,exist_ok=True)
    name='WRO_HUB_GERBER_20261028_SYNC_FIXED.zip'
    target=OUTPUT/name
    if target.exists() and target.read_bytes()!=zipbytes:raise FileExistsError(target)
    target.write_bytes(zipbytes)
    shutil.copyfile(AFTER,OUTPUT/'WRO_HUB_PROJECT_20261028_SYNC_FIXED.epro2')
    rawbom=base64.b64decode(capture['bom']['b64'])
    text=rawbom.decode('utf-16') if rawbom[:2] in (b'\xff\xfe',b'\xfe\xff') else rawbom.decode('utf-8-sig')
    (OUTPUT/'WRO_HUB_RAW_BOM_20261028_SYNC_FIXED.csv').write_text(text,encoding='utf-8-sig')
    (OUTPUT/'SYNC-VERIFICATION.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    manifest={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUTPUT.iterdir() if p.is_file() and p.name!='SHA256SUMS.txt'}
    (OUTPUT/'SHA256SUMS.txt').write_text(''.join(f'{sha}  {name}\n' for name,sha in sorted(manifest.items())),encoding='utf-8')
    print(json.dumps(dict(drc=live['drc'],erc=erc['erc'],metadataOnlyPropertyCount=25,
                          allImageAndPhysicalRecordsPreserved=True,gerber=str(target),
                          rasterArtworkPreservedButNotInStandardGerber=True,sha256=manifest[name]),indent=1))


if __name__=='__main__':main()
