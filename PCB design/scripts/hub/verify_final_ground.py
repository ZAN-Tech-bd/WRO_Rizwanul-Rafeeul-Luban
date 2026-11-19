"""Full-export, saved-copper, connectivity and no-unapproved-edit gate for the final ground review."""
import json
from pathlib import Path
import sys

import final_ground as g
from test_sb_removal import archive,live,PCB


def verify(path,snapshot=None):
    before,after=archive(g.BEFORE),archive(path)
    assert before.keys()==after.keys()
    b,a=before[PCB]['records'],after[PCB]['records']
    assert not b.keys()-a.keys(),'Original design objects deleted'
    additions=a.keys()-b.keys()
    assert all(k[0] in {'FILL','ELE_PLACEHOLDER'} for k in additions),'Unexpected new design objects'
    moved=[];labels=[]
    for key,r in b.items():
        old,new=r['body'],a[key]['body'];kind,rid=key
        if kind=='VIA' and rid in g.VIA_MOVES:
            assert old['netName']==new['netName']=='GND'
            assert (new['centerX'],new['centerY'])==g.VIA_MOVES[rid]
            rest_old={k:v for k,v in old.items() if k not in {'centerX','centerY'}}
            rest_new={k:v for k,v in new.items() if k not in {'centerX','centerY'}}
            for k in set(rest_old)|set(rest_new):
                a_,b_=rest_old.get(k),rest_new.get(k)
                if isinstance(a_,float) and isinstance(b_,float):assert abs(a_-b_)<=.11,'via field '+k
                else:assert a_==b_,'via field '+k
            moved.append(rid)
        elif key==('STRING','49529d11268779e9'):
            assert old['text']=='-  5V FROM BUCK  +' and new['text']=='+  5V FROM BUCK  -'
            assert {k:v for k,v in old.items() if k!='text'}=={k:v for k,v in new.items() if k!='text'}
            labels.append(rid)
        elif kind!='ELE_PLACEHOLDER':assert r['raw']==a[key]['raw'],'Unapproved modification '+str(key)
    for uuid in before.keys()-{PCB}:
        assert before[uuid]['records']==after[uuid]['records'],'Other project document changed '+uuid
    native_fills=[r['body'] for (kind,_),r in a.items() if kind=='FILL']
    assert len(native_fills)==len(g.load_plan()['pieces'])
    for f in native_fills:
        assert f['netName']=='GND' and f['layerId'] in (1,2) and f['fillStyle']=='SOLID'
    planned=g.load_plan();original=planned['pieces']
    try:
        # Run the geometric checker on the actual saved polygons, not just the plan.
        planned['pieces']=[{'layer':f['layerId'],'source':f['path']} for f in native_fills]
        old_loader=g.load_plan;g.load_plan=lambda:planned
        actual=g.verify_plan()
    finally:
        g.load_plan=old_loader
    assert not actual['foreignOverlaps'] and not actual['clearanceViolations'] and not actual['disconnectedGnd'],actual
    report={'sourceArchive':str(g.BEFORE),'verifiedArchive':str(path),'newGndPieces':len(native_fills),
            'relocatedGndStitchIds':moved,'correctedPolarityLegends':len(labels),
            'allComponentsTracksPadsNetsFootprintsOutlineAndSchematicPreserved':True,'actualCopperCheck':actual}
    if snapshot:
        s=live(snapshot)
        assert s['drc']==[],'Strict DRC still has findings'
        assert len(s['components'])==77 and len(s['nets'])==74
        report['strictDrcErrors']=0
    (g.DATA/'final_verification.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=1))


if __name__=='__main__':verify(Path(sys.argv[1]),Path(sys.argv[2]) if len(sys.argv)>2 else None)
