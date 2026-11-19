"""H-004: R4/R6/R13 plus existing station/title silk only; no whole-board regeneration."""
import copy
import json
import math
from pathlib import Path
import re

from live_space import parse_inventory, board, box, box_in_poly
from station_placement import transform, net_pad
from test_sb_removal import archive, live, PCB

HERE=Path(__file__).resolve().parent
DATA=HERE/'data/station_cleanup_20261027'
BACKUP=HERE.parents[1]/'hub/WRO_HUB_before_station_cleanup_guard_20261027-0209.epro2'
POSES={'R4':(2310,1805,90),'R6':(2145,1805,90),'R13':(2680,1905,90)}
DESIGNATORS={'R4':(2425,1805),'R6':(2040,1805),'R13':(2680,2190)}


def parts():
    return parse_inventory(DATA/'before_inventory.json')


def final_parts():
    result=parts()
    for d,pose in POSES.items():
        result[d]=transform(result[d],pose)
    return result


def body_problems():
    result=final_parts()
    outline,holes=board(BACKUP)
    boxes={d:box(p) for d,p in result.items()}
    errors=[]
    for d in POSES:
        a=boxes[d]
        if not box_in_poly(a,outline,-10):errors.append(d+' off board')
        for other,b in boxes.items():
            if d!=other and min(a[2],b[2])-max(a[0],b[0])>-20 and min(a[3],b[3])-max(a[1],b[1])>-20:
                errors.append(d+' overlaps '+other)
        for hid,x,y,r in holes:
            dx=max(a[0]-x,0,x-a[2]);dy=max(a[1]-y,0,y-a[3])
            if math.hypot(dx,dy)<r+40:errors.append(d+' near '+hid)
    return errors


def text_plan():
    recs=archive(BACKUP)[PCB]['records']
    strings=[(rid,v['body']) for (t,rid),v in recs.items() if t=='STRING']
    plan={}
    current=parts()
    for i,(rid,b) in enumerate(strings):
        match=re.fullmatch(r'S([1-8])\s+Z([1-4])',b['text'])
        if not match:continue
        k=int(match[1]); port=current[f'J{k+9}']; cx,cy=port['x'],port['y']
        assert float(port['rot'])==90, 'Connector geometry changed: re-plan'
        name_y=cy+500 if k<=4 else cy-520
        plan[rid]={'kind':'station','text':b['text'],'x':cx-64,'y':name_y,'rotation':0,
                   'fontSize':40,'lineWidth':6}
        labels=strings[i+1:i+9]
        expected=['5V','GND','3V3','DHT','MQ','PRX','DI','DO'] if k<8 else ['5V','GND','3V3','nc','nc','PRX','DI','DO']
        assert [q['text'] for _,q in labels]==expected, f'S{k}: unexpected original label identities/order'
        for number,(sid,q) in enumerate(labels,1):
            _,dx,dy=port['pads'][str(number)]
            label_y=cy+dy-(20 if k==5 and number==1 else 0)  # Clear R64's lower pad at the row turn.
            plan[sid]={'kind':'pin','text':q['text'],'x':cx+165,'y':label_y,'rotation':0,
                       'fontSize':32,'lineWidth':6}
    title=[(rid,b) for rid,b in strings if b['text']=='MUSEUM HUB PCB v0.4']
    assert len(title)==1
    rid,b=title[0]
    plan[rid]={'kind':'title','text':b['text'],'x':3560,'y':620,'rotation':0,
               'fontSize':b['fontSize'],'lineWidth':b['strokeWidth']}
    assert len(plan)==73
    return plan


def proposed_silk():
    recs=archive(BACKUP)[PCB]['records']
    snap=live(DATA/'before_silk.json')
    raw={rid:v['body'] for (t,rid),v in recs.items() if t in {'STRING','ATTR'}}
    plan=text_plan()
    result=[]
    for row in snap['texts']+snap['designators']:
        if not row['box']:continue
        rid=row['id']; b=copy.deepcopy(row['box'])
        if rid in plan:
            old=raw[rid]; q=plan[rid]; ratio=q['fontSize']/old['fontSize']
            # Current station/title strings are all at rotation 0. Keep their original alignment.
            b={key:q['x' if key.endswith('X') else 'y']+(value-old['x' if key.endswith('X') else 'y'])*ratio
               for key,value in b.items()}
        else:
            old=raw.get(rid,{})
            parent=old.get('parentId')
            des=next((v['body']['value'] for (t,_),v in recs.items()
                      if t=='ATTR' and v['body'].get('parentId')==parent and v['body'].get('key')=='Designator'),None)
            if des in POSES:
                # Explicit horizontal labels adjacent to, rather than under, the resistor body.
                old_width=b['maxX']-b['minX'];old_height=b['maxY']-b['minY']
                if old.get('angle',0)%180:
                    old_width,old_height=old_height,old_width
                ratio=32/old.get('fontSize',30)
                x,y=DESIGNATORS[des];w,h=old_width*ratio,old_height*ratio
                b=dict(minX=x-w/2,maxX=x+w/2,minY=y-h/2,maxY=y+h/2)
        result.append(dict(row,box=b,moved=rid in plan or (rid in raw and raw[rid].get('key')=='Designator' and raw[rid].get('value') in DESIGNATORS)))
    return result


def silk_problems():
    rows=proposed_silk();snap=live(DATA/'before_silk.json');oldparts=parts();newparts=final_parts()
    pads=copy.deepcopy(snap['pads'])
    oldlive=live(DATA/'before_live.json')
    byid={p['id']:(c['designator'],p) for c in oldlive['components'] for p in c['pads']}
    for p in pads:
        owner=byid.get(p['id'])
        if owner and owner[0] in POSES:
            des,pin=owner; old,new=oldparts[des],newparts[des]
            _,dx,dy=new['pads'][pin['number']]
            p['x'],p['y']=new['x']+dx,new['y']+dy
    problems=[]
    for a in rows:
        if not a['moved']:continue
        b=a['box']
        if b['minX']<-665 or b['maxX']>4685 or b['minY']<10 or b['maxY']>3965:
            problems.append(a['text']+' off edge')
        for p in pads:
            radius=max(p['pad'][1:3])/2+6
            dx=max(b['minX']-p['x'],0,p['x']-b['maxX']);dy=max(b['minY']-p['y'],0,p['y']-b['maxY'])
            if math.hypot(dx,dy)<radius:problems.append(a['text']+' pad '+p['id'])
        for other in rows:
            if a['id']==other['id']:continue
            q=other['box']
            if min(b['maxX'],q['maxX'])-max(b['minX'],q['minX'])>2 and min(b['maxY'],q['maxY'])-max(b['minY'],q['minY'])>2:
                problems.append(a['text']+' overlaps '+other['text'])
    return problems


if __name__=='__main__':
    plan={'poses':POSES,'texts':text_plan(),'bodyProblems':body_problems(),'silkProblems':silk_problems()}
    (DATA/'cleanup_plan.json').write_text(json.dumps(plan,indent=2))
    print(json.dumps({k:v for k,v in plan.items() if k!='texts'},indent=1));print('Text targets',len(plan['texts']))
    raise SystemExit(bool(plan['bodyProblems'] or plan['silkProblems']))
 
