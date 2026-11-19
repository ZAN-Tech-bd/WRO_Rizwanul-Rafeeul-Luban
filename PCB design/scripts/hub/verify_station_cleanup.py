"""H-004 full-export allowlist + actual silk-clearance/DRC acceptance.
Run: python verify_station_cleanup.py AFTER.epro2 [AFTER_LIVE.json AFTER_SILK.json]
"""
import collections
import json
import math
from pathlib import Path
import sys

from station_cleanup import BACKUP,DATA,POSES,DESIGNATORS,text_plan
from test_sb_removal import archive,live,PCB


def verify(after_path, live_path=None, silk_path=None):
    before,after=archive(BACKUP),archive(after_path)
    assert before.keys()==after.keys()
    b,a=before[PCB]['records'],after[PCB]['records']
    assert b.keys()==a.keys(),'Record addition/deletion'
    des={v['body']['parentId']:v['body']['value'] for (t,_),v in b.items() if t=='ATTR' and v['body'].get('key')=='Designator'}
    moving={cid:d for cid,d in des.items() if d in POSES}
    texts=text_plan();component_changes=[];text_changes=[];attribute_changes=[]
    for key,r in b.items():
        kind,rid=key;old,new=r['body'],a[key]['body'];allowed=set()
        if kind=='COMPONENT' and rid in moving:
            d=moving[rid];x,y,rot=POSES[d]
            assert (new['x'],new['y'])==(x,y) and new['angle']%360==rot%360,d+' target not reached'
            allowed={'x','y','angle'}
            component_changes.append(d)
        elif kind=='ATTR' and old.get('parentId') in moving:
            allowed={'x','y','angle'}
            if old.get('key')=='Designator':
                d=old['value'];x,y=DESIGNATORS[d]
                assert (new['x'],new['y'])==(x,y) and new['angle']%360==0
                assert new['fontSize']==32 and new['strokeWidth']==6
                allowed|={'fontSize','strokeWidth'}
            if old!=new:attribute_changes.append(rid)
        elif kind=='STRING' and rid in texts:
            t=texts[rid]
            assert (new['x'],new['y'])==(t['x'],t['y']) and new['angle']==0,t['text']+' not placed'
            assert new['fontSize']==t['fontSize'] and new['strokeWidth']==t['lineWidth']
            allowed={'x','y','angle','fontSize','strokeWidth'}
            text_changes.append(rid)
        if allowed:
            assert {k:v for k,v in old.items() if k not in allowed}=={k:v for k,v in new.items() if k not in allowed},str(key)+' unexpected property'
        else:assert r['raw']==a[key]['raw'],'Unapproved PCB change '+str(key)
    assert set(component_changes)==set(POSES) and len(text_changes)==73
    for uuid in before.keys()-{PCB}:
        assert before[uuid]['records']==after[uuid]['records'],'Other document changed '+uuid
    report={'componentsMoved':component_changes,'freeTextsMoved':len(text_changes),'ownedAttributesAdjusted':len(attribute_changes),
            'otherComponentsFrozen':74,'allOtherPcbAndProjectRecordsUnchanged':True}
    if live_path:
        old,new=live(DATA/'before_live.json'),live(live_path)
        assert new['nets']==old['nets'] and len(new['components'])==77
        def leaves(x):
            if isinstance(x,list):return [q for z in x for q in leaves(z)]
            if isinstance(x,dict) and 'list' in x:return leaves(x['list'])
            return [x] if isinstance(x,dict) and 'errorType' in x else []
        def sig(e):return json.dumps({k:e.get(k) for k in ['errorType','errorObjType','ruleName','net','obj1','obj2','objs']},sort_keys=True)
        assert not collections.Counter(map(sig,leaves(new['drc'])))-collections.Counter(map(sig,leaves(old['drc']))),'New DRC finding'
        report['drcBefore']=[(x['name'],x['count']) for x in old['drc']]
        report['drcAfter']=[(x['name'],x['count']) for x in new['drc']]
        report['newDrcFindings']=0
    if silk_path:
        silk=live(silk_path)
        rows=silk['texts']+silk['designators']
        label_ids=set(texts)|{rid for (t,rid),r in b.items() if t=='ATTR' and r['body'].get('key')=='Designator' and r['body'].get('value') in POSES}
        errors=[]
        for row in rows:
            if row['id'] not in label_ids:continue
            box=row['box'];assert box,'Missing silk bbox'
            if box['minX']<-665 or box['maxX']>4685 or box['minY']<10 or box['maxY']>3965:errors.append(row['text']+' off edge')
            for p in silk['pads']:
                r=max(p['pad'][1:3])/2+6
                dx=max(box['minX']-p['x'],0,p['x']-box['maxX']);dy=max(box['minY']-p['y'],0,p['y']-box['maxY'])
                if math.hypot(dx,dy)<r:errors.append(row['text']+' clips pad '+p['id'])
            for other in rows:
                q=other['box']
                if row['id']==other['id'] or not q:continue
                if min(box['maxX'],q['maxX'])-max(box['minX'],q['minX'])>2 and min(box['maxY'],q['maxY'])-max(box['minY'],q['minY'])>2:
                    errors.append(row['text']+' overlaps '+other['text'])
        assert errors==[],str(errors)
        report['checkedMovedSilkObjects']=len(label_ids);report['silkPadEdgeOverlapProblems']=0
    (DATA/'verification.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=1))


if __name__=='__main__':verify(Path(sys.argv[1]),*(Path(x) for x in sys.argv[2:]))
