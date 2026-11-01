"""Utilities pinned to the user-authorized isolated repair working copy, never another project."""
import json
from pathlib import Path
import zipfile

HERE=Path(__file__).resolve().parent
DATA=HERE/'data/blocker_fix_20261027'
TARGET=json.loads((DATA/'target.json').read_text())


def read_archive(path):
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        names=[n for n in z.namelist() if n.endswith('.epru')]
        assert len(names)==1
        text=z.read(names[0]).decode('utf-8')
    decoder=json.JSONDecoder();docs={};current=None
    for line in text.splitlines():
        left,sep,right=line.partition('||')
        if not sep or not right.startswith('{'):continue
        header=json.loads(left);body,end=decoder.raw_decode(right)
        if header['type']=='DOCHEAD':
            current={'head':body,'records':{}}
            docs[body['uuid']]=current
        elif current is not None:
            key=(header['type'],header['id'])
            assert key not in current['records']
            current['records'][key]={'body':body,'raw':right[:end]}
    assert TARGET['pcb'] in docs
    return docs


def json_value(path):
    value=json.loads(Path(path).read_text(encoding='utf-8'))
    return json.loads(value) if isinstance(value,str) else value


def compare_repour_records(before,after,pcb):
    """Permit fills/allocators and track re-identification, never a copper/body/net change."""
    from collections import Counter
    assert set(before)==set(after),'Project document set changed'
    body=lambda rec:json.dumps(rec['body'],sort_keys=True,separators=(',',':'),ensure_ascii=False)
    for doc in before:
        a=before[doc]['records'];b=after[doc]['records']
        if doc==pcb:
            la={k[1]:r for k,r in a.items() if k[0]=='LINE'}
            lb={k[1]:r for k,r in b.items() if k[0]=='LINE'}
            assert Counter(map(body,la.values()))==Counter(map(body,lb.values())),'Track body multiset changed'
            ignored={'LINE','FILL','ELE_PLACEHOLDER'}
            a={k:r for k,r in a.items() if k[0] not in ignored}
            b={k:r for k,r in b.items() if k[0] not in ignored}
        assert set(a)==set(b),f'Non-fill record set changed in {doc}'
        assert all(a[k]['body']==b[k]['body'] for k in a),f'Non-fill record bodies changed in {doc}'
    old=before[pcb]['records'];new=after[pcb]['records']
    la={k[1] for k in old if k[0]=='LINE'};lb={k[1] for k in new if k[0]=='LINE'}
    return dict(removedTrackIds=sorted(la-lb),addedTrackIds=sorted(lb-la),
                addedFillCount=sum(t=='FILL' and (t,i) not in old for t,i in new),
                preservedNonFillBodies=True,allProjectDocumentsCompared=len(before))


def read_metric_drills(text):
    """Read this exporter’s explicit-decimal metric Excellon tools, hits and G85 slots."""
    import re
    if not re.search(r'^METRIC',text,re.M) or re.search(r'^INCH',text,re.M):
        raise ValueError('Expected explicit metric drill output')
    tools={};active=None;rounds=[];slots=[]
    number=r'([+-]?\d+(?:\.\d+)?)'
    for line in text.splitlines():
        line=line.strip()
        tool=re.fullmatch(r'T(\d+)C'+number,line)
        if tool:tools[int(tool[1])]=float(tool[2]);continue
        select=re.fullmatch(r'T(\d+)',line)
        if select:active=tools[int(select[1])];continue
        if not line.startswith('X'):continue
        if active is None:raise ValueError('Coordinate without selected drill tool')
        slot=re.fullmatch('X'+number+'Y'+number+'G85X'+number+'Y'+number,line)
        if slot:
            slots.append(dict(start=(float(slot[1]),float(slot[2])),end=(float(slot[3]),float(slot[4])),diameter=active))
            continue
        hit=re.fullmatch('X'+number+'Y'+number,line)
        if not hit:raise ValueError('Unmodelled drill command '+line)
        rounds.append(dict(x=float(hit[1]),y=float(hit[2]),diameter=active))
    return dict(round=rounds,slots=slots,tools=tools,plated=';TYPE=PLATED' in text)


def match_slot_geometries(expected_shapes,actual_slots,tolerance=.003):
    """Match complete metric slot capsules bijectively, including orientation and position."""
    from shapely.geometry import LineString
    assert len(expected_shapes)==len(actual_slots),'Slot count differs from native design'
    actual=[LineString([s['start'],s['end']]).buffer(s['diameter']/2,quad_segs=64) for s in actual_slots]
    consumed=set();assignment=[]
    for expected in expected_shapes:
        matches=[j for j,shape in enumerate(actual) if j not in consumed and expected.hausdorff_distance(shape)<tolerance]
        assert len(matches)==1,'Missing, reused or geometrically ambiguous plated slot'
        assignment.append(matches[0]);consumed.add(matches[0])
    assert len(consumed)==len(actual),'Unmatched manufactured slots'
    return assignment


def canonical_gerber_operations(text):
    """Normalize this exporter's aperture IDs/unused definitions, retaining all used geometry.

    This is an equality comparison, not a renderer or a general Gerber validator. All coordinate,
    macro, unit, polarity and drawing commands are retained; only comments and ADD declarations
    are removed and aperture selections are replaced by their complete definition.
    """
    import re
    definitions={int(code):value for code,value in re.findall(r'%ADD(\d+)([^%]+)%',text)}
    result=[]
    for line in text.splitlines():
        line=line.strip()
        if not line or line.startswith('G04 ') or line.startswith('%ADD'):continue
        selected=re.fullmatch(r'(?:G54)?D(\d+)\*',line)
        if selected and int(selected[1])>=10:
            code=int(selected[1]);assert code in definitions,'Undefined selected Gerber aperture'
            result.append('APERTURE='+definitions[code])
        else:result.append(line)
    return tuple(result)


def silk_style_plan(records):
    """Only enlarge visible silk; preserve all text identities and hidden metadata."""
    result=[]
    for (kind,rid),record in records.items():
        b=record['body']
        if kind not in {'STRING','ATTR'} or b.get('layerId') not in (3,4):continue
        if kind=='ATTR' and not b.get('valueVisible'):continue
        size=max(40,b.get('fontSize',0));width=max(6,b.get('strokeWidth',0))
        if size==b.get('fontSize') and width==b.get('strokeWidth'):continue
        result.append(dict(kind=kind,id=rid,text=b['text'] if kind=='STRING' else b['value'],
                           fontSize=size,lineWidth=width))
    return result


def build_silk_style(archive_path):
    records=read_archive(archive_path)[TARGET['pcb']]['records']
    plan=silk_style_plan(records)
    (DATA/'silk_style_plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
    source=(HERE/'75_normalize_repair_silk.template.js').read_text(encoding='utf-8')
    source=source.replace('__TARGET__',json.dumps(TARGET)).replace('__SILK_STYLE_PLAN__',json.dumps(plan))
    (HERE/'75_normalize_repair_silk.js').write_text(source,encoding='utf-8')
    return plan


def copper_model(archive_path):
    """Exact saved native geometry, including current footprint dimensions and static fills."""
    from shapely.geometry import Point,LineString,box
    from final_ground import transform,pad_shape,from_source,QS
    docs=read_archive(archive_path);records=docs[TARGET['pcb']]['records']
    components={rid:r['body'] for (t,rid),r in records.items() if t=='COMPONENT'}
    attrs={cid:{} for cid in components}
    for (t,_),r in records.items():
        b=r['body']
        if t=='ATTR' and b.get('parentId') in attrs:attrs[b['parentId']][b['key']]=b['value']
    nets={(json.loads(rid)[1],json.loads(rid)[3]):r['body']['padNet']
          for (t,rid),r in records.items() if t=='PAD_NET'}
    pads=[];holes=[];items=[];fills=[]
    def add_pad(p,des,rid,cx=0,cy=0,rot=0,net=''):
        center=transform(Point(p['centerX'],p['centerY']),cx,cy,rot)
        angle=rot+p.get('padAngle',0);copper=transform(pad_shape(p),center.x,center.y,angle)
        hd=p.get('hole') or {};hole=None
        if hd.get('holeType')=='ROUND':hole=Point(0,0).buffer(hd['width']/2,quad_segs=QS)
        elif hd.get('holeType')=='SLOT':
            length,width=hd['width'],hd['height']
            hole=LineString([(-(length-width)/2,0),((length-width)/2,0)]).buffer(width/2,quad_segs=QS)
        if hole is not None:
            hole=transform(hole,center.x,center.y,angle+p.get('relativeAngle',0))
            holes.append(dict(des=des,num=p['num'],shape=hole,net=net))
        layers=(1,2) if p['layerId']==12 else (p['layerId'],)
        pads.append(dict(des=des,num=p['num'],id=rid,net=net,shape=copper,hole=hole,
                         center=(center.x,center.y),layers=layers,native=p))
        items.append(dict(kind='pad',id=rid,net=net,layers=layers,shape=copper,label=des+'.'+p['num']))
    for cid,c in components.items():
        for (t,rid),r in docs[attrs[cid]['Footprint']]['records'].items():
            if t=='PAD':add_pad(r['body'],attrs[cid]['Designator'],cid+rid,c['x'],c['y'],c['angle'],nets.get((cid,rid),''))
    for (t,rid),r in records.items():
        b=r['body']
        if t=='PAD':add_pad(b,'MH',rid,net=b.get('netName',''))
        elif t=='LINE' and b.get('layerId') in (1,2):
            a=(b['startX'],b['startY']);z=(b['endX'],b['endY']);g=Point(a) if a==z else LineString([a,z])
            items.append(dict(kind='track',id=rid,net=b['netName'],layers=(b['layerId'],),
                              shape=g.buffer(b['width']/2,quad_segs=QS),label=b['netName']+' track '+rid))
        elif t=='VIA':
            xy=(b['centerX'],b['centerY'])
            items.append(dict(kind='via',id=rid,net=b['netName'],layers=(1,2),
                              shape=Point(xy).buffer(b['viaDiameter']/2,quad_segs=QS),label=b['netName']+' via '+rid))
            holes.append(dict(des='via',num=rid,shape=Point(xy).buffer(b['holeDiameter']/2,quad_segs=QS),net=b['netName']))
        elif t=='FILL':fills.append(dict(id=rid,layer=b['layerId'],source=b['path'],shape=from_source(b['path'])))
        elif t in {'ARC','POLY','REGION'} and b.get('layerId') in (1,2):
            raise ValueError('Unmodelled copper primitive '+t+' '+rid)
    outline=next(from_source(r['body']['path']) for (t,_),r in records.items() if t=='POLY' and r['body'].get('layerId')==11)
    u=next(p for p in pads if p['des']=='U1A' and p['num']=='1');x,y=u['center']
    return dict(docs=docs,records=records,components=components,attrs=attrs,pads=pads,holes=holes,
                items=items,fills=fills,outline=outline,antenna=box(x-60,y-175,x+1060,outline.bounds[3]+100),
                padNetMismatches=[])


def silk_rows(model,snapshot):
    result=[]
    for kind,key in [('STRING','texts'),('ATTR','designators')]:
        for q in snapshot[key]:
            b=model['records'].get((kind,q['id']),{}).get('body',{})
            if b.get('layerId') not in (3,4) or not q.get('box'):continue
            if kind=='ATTR' and not b.get('valueVisible'):continue
            result.append(dict(q,kind=kind,x=b['x'],y=b['y'],layer=b['layerId']))
    return result


def place_silk(rows,model):
    """Keep legal labels fixed, move conflicting labels the smallest bounded translation."""
    import copy,math
    from shapely.geometry import box
    from shapely.ops import unary_union
    obstacles={layer:unary_union([p['shape'].buffer(6.5) for p in model['pads']
                                  if (1 if layer==3 else 2) in p['layers']]) for layer in (3,4)}
    allowed=model['outline'].buffer(-16)
    fixed={};pending=[];occupied={3:[],4:[]}
    def shape(row):
        b=row['box'];return box(b['minX'],b['minY'],b['maxX'],b['maxY'])
    def legal(row):
        s=shape(row);layer=row['layer']
        return allowed.covers(s) and not s.intersects(obstacles[layer]) and all(s.distance(q)>=6 for q in occupied[layer])
    # Preserve existing valid labels before searching for any replacement positions.
    for i,row in enumerate(rows):
        if legal(row):fixed[i]=copy.deepcopy(row);occupied[row['layer']].append(shape(row))
        else:pending.append(i)
    offsets=sorted([(x,y) for x in range(-300,301,10) for y in range(-300,301,10)],
                   key=lambda xy:(math.hypot(*xy),abs(xy[1]),xy))
    for i in pending:
        row=rows[i]
        for dx,dy in offsets:
            if row['text'] in {'+','-'} and math.hypot(dx,dy)>100:continue
            q=copy.deepcopy(row);q['x']+=dx;q['y']+=dy
            q['box']={k:v+(dx if k.endswith('X') else dy) for k,v in row['box'].items()}
            if legal(q):fixed[i]=q;occupied[q['layer']].append(shape(q));break
        else:raise ValueError('No bounded silk position for '+row['text']+' '+row['id'])
    return [fixed[i] for i in range(len(rows))]


def build_silk_positions(archive_path,snapshot_path):
    model=copper_model(archive_path);rows=silk_rows(model,json_value(snapshot_path));placed=place_silk(rows,model)
    plan=[dict(kind=a['kind'],id=a['id'],text=a['text'],before=[a['x'],a['y']],x=b['x'],y=b['y'])
          for a,b in zip(rows,placed) if a['x']!=b['x'] or a['y']!=b['y']]
    (DATA/'silk_position_plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
    source=(HERE/'76_relocate_repair_silk.template.js').read_text(encoding='utf-8')
    source=source.replace('__TARGET__',json.dumps(TARGET)).replace('__PLAN__',json.dumps(plan))
    (HERE/'76_relocate_repair_silk.js').write_text(source,encoding='utf-8')
    return plan


def rf_plan(model):
    """Smallest full-width detour below the module's actual radiating extension."""
    old={'3b4f9ab3221e7d4d':(2250,3810,2355,3915),
         '84b60016a78b71de':(2355,3915,2680,3915)}
    for rid,xy in old.items():
        b=model['records'][('LINE',rid)]['body']
        actual=(b['startX'],b['startY'],b['endX'],b['endY'])
        if actual!=xy or b['netName']!='+3V3' or b['layerId']!=2 or abs(b['width']-31.4961)>.001:
            raise ValueError('RF source changed; re-plan')
    def line(x1,y1,x2,y2):return dict(net='+3V3',layer=2,x1=x1,y1=y1,x2=x2,y2=y2,width=31.4961)
    return dict(modify={'3b4f9ab3221e7d4d':line(2250,3810,2305,3865),
                        '84b60016a78b71de':line(2305,3865,2630,3865)},
                add=[line(2630,3865,2680,3915)],before={rid:dict(zip(['x1','y1','x2','y2'],xy)) for rid,xy in old.items()})


def build_rf_writer(archive_path):
    m=copper_model(archive_path);plan=rf_plan(m)
    (DATA/'rf_plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
    source=(HERE/'77_repair_antenna_route.template.js').read_text(encoding='utf-8')
    source=source.replace('__TARGET__',json.dumps(TARGET)).replace('__PLAN__',json.dumps(plan))
    (HERE/'77_repair_antenna_route.js').write_text(source,encoding='utf-8')
    return plan


def check_ground_pieces(model,pieces):
    """Reuse the verified pure geometric gate, never its historical input reader."""
    import final_ground as engine
    old_model,old_plan=engine.load_model,engine.load_plan
    try:
        engine.load_model=lambda:model
        engine.load_plan=lambda:dict(pieces=pieces)
        return engine.verify_plan()
    finally:engine.load_model,engine.load_plan=old_model,old_plan


def regenerate_ground(archive_path):
    """Run the existing reviewed algorithm with explicit current inputs; restore module context."""
    import final_ground as engine
    m=copper_model(archive_path);output=DATA/'ground_repair';output.mkdir(exist_ok=True)
    old=(engine.load_model,engine.DATA,engine.BEFORE)
    try:
        engine.load_model=lambda:m;engine.DATA=output;engine.BEFORE=Path(archive_path)
        engine.generate()
        plan=engine.load_plan();report=engine.verify_plan()
    finally:engine.load_model,engine.DATA,engine.BEFORE=old
    return plan,report


def build_ground_writers(archive_path,snapshot_path):
    m=copper_model(archive_path);snap=json_value(snapshot_path)
    pieces=json.loads((DATA/'ground_repair/ground_plan.json').read_text())['pieces']
    report=check_ground_pieces(m,pieces)
    if report['foreignOverlaps'] or report['clearanceViolations'] or report['disconnectedGnd']:
        raise ValueError('Ground plan failed current-model gate')
    lines=[dict(id=rid,net=b['netName'],layer=b['layerId'],x1=b['startX'],y1=b['startY'],x2=b['endX'],y2=b['endY'],width=b['width'])
           for (t,rid),r in m['records'].items() for b in [r['body']] if t=='LINE' and b.get('layerId') in (1,2)]
    vias=[dict(id=rid,net=b['netName'],x=b['centerX'],y=b['centerY'],hole=b['holeDiameter'],diameter=b['viaDiameter'])
          for (t,rid),r in m['records'].items() for b in [r['body']] if t=='VIA']
    guard=(HERE/'repair_copper_guard.js').read_text(encoding='utf-8')
    config=dict(target=TARGET,components=snap['components'],lines=lines,vias=vias)
    for number,name,data in [('78','clear',m['fills']),('79','add',pieces)]:
        rows=[dict(id=f['id'],layer=f['layer'],source=f['source']) for f in data] if name=='clear' else data
        template=(HERE/f'{number}_{name}_repair_ground.template.js').read_text(encoding='utf-8')
        source=template.replace('__GUARD__',guard).replace('__CONFIG__',json.dumps(dict(config,polygons=rows),separators=(',',':')))
        (HERE/f'{number}_{name}_repair_ground.js').write_text(source,encoding='utf-8')
    return report


def switch_metadata_package(source,destination):
    """Patch only J3's saved metadata offline; never call the defective library modify API."""
    import copy
    destination=Path(destination)
    if destination.exists():raise FileExistsError(destination)
    docs=read_archive(source);instances={}
    for doc in (TARGET['pcb'],TARGET['page']):
        instances[doc]=next(r['body']['parentId'] for (t,_),r in docs[doc]['records'].items()
                            if t=='ATTR' and r['body'].get('key')=='Designator' and r['body'].get('value')=='J3')
    sch=docs[TARGET['page']]['records']
    attrs={r['body']['key']:r['body']['value'] for (t,_),r in sch.items()
           if t=='ATTR' and r['body'].get('parentId')==instances[TARGET['page']]}
    device=attrs['Device']
    # Fail closed if another component shares this device; the intended library edit is J3-only.
    users=[r['body']['parentId'] for doc in (TARGET['page'],TARGET['pcb'])
           for (t,_),r in docs[doc]['records'].items()
           if t=='ATTR' and r['body'].get('key')=='Device' and r['body'].get('value')==device]
    if sorted(users)!=sorted(instances.values()):raise ValueError('J3 device is shared or missing')
    description=('User-supplied KCD4-203-B DPDT centre-off rocker; custom direct-mount plated-slot footprint. '
                 'Actual-part fit, mechanical support and contact sequence require hardware verification.')
    values={'Name':'KCD4-203-B','Manufacturer':'','Manufacturer Part':'KCD4-203-B',
            'Supplier':'','Supplier Part':'','LCSC Part Name':'','JLCPCB Part Class':'',
            'Supplier Footprint':'Custom KCD4-203-B plated-slot footprint','Description':description,
            'Pins Structure':'2x3P','Pitch':'10.5mm','Row Spacing':'11.0mm',
            'Mounting Style':'Through-hole plated slots','Reference Series':'KCD4',
            'Number of Pins':'6P','Number of Rows':'2','Number of PINs Per Row':'3'}
    preserved={'Symbol','Device','Unique ID','Add into BOM','Convert to PCB','Designator','Footprint',
               'Reuse Block','Group ID','Channel ID'}
    updates={}
    for doc,cid in instances.items():
        for key,r in docs[doc]['records'].items():
            b=r['body']
            if key[0]!='ATTR' or b.get('parentId')!=cid or b['key'] in preserved:continue
            desired=values.get(b['key'],'')
            if b['value']!=desired:
                new=copy.deepcopy(b);new['value']=desired;updates[(doc,*key)]=new
    meta=copy.deepcopy(docs[device]['records'][('META','META')]['body'])
    meta.update(title='KCD4-203-B',tags=['Switches','DPDT centre-off'],source='',images=[])
    meta['attributes']=dict(values,Symbol=attrs['Symbol'],Footprint=TARGET['footprint'],Designator='J?',
                            **{'Add into BOM':'yes','Convert to PCB':'yes'})
    updates[(device,'META','META')]=meta
    with zipfile.ZipFile(source) as z:files={n:z.read(n) for n in z.namelist()}
    native=next(n for n in files if n.endswith('.epru'));decoder=json.JSONDecoder();current=None;lines=[];seen=[]
    for line in files[native].decode('utf-8').splitlines(keepends=True):
        left,sep,right=line.partition('||')
        if not sep or not right.startswith('{'):lines.append(line);continue
        h=json.loads(left);b,end=decoder.raw_decode(right)
        if h['type']=='DOCHEAD':current=b['uuid']
        key=(current,h['type'],h.get('id'))
        if key in updates:
            line=left+'||'+json.dumps(updates[key],ensure_ascii=False,separators=(',',':'))+right[end:];seen.append(key)
        lines.append(line)
    if set(seen)!=set(updates):raise ValueError('Not every targeted metadata record was patched')
    files[native]=''.join(lines).encode('utf-8')
    with zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED) as z:
        for name,data in files.items():z.writestr(name,data)
    with zipfile.ZipFile(destination) as z:assert z.testzip() is None
    return dict(deviceUuid=device,j3InstanceIds=list(instances.values()),changedRecords=[list(k) for k in updates],
                allOtherRecordBodiesBytePreserved=True)


def build_scoped_readers():
    replacements={'WRO_HUB':TARGET['name'],'ad35aed4d7610fd2':TARGET['pcb'],'7e64f4e0d47bf001':TARGET['page']}
    scripts={'55_sb_snapshot.js':'71_repair_pcb_snapshot.js','60_cleanup_silk_snapshot.js':'72_repair_silk_snapshot.js',
             '29_dump_copper.js':'73_repair_copper_snapshot.js','20e_live_pads.js':'74_repair_pad_snapshot.js'}
    for source,dest in scripts.items():
        text=(HERE/source).read_text(encoding='utf-8')
        for old,new in replacements.items():text=text.replace(old,new)
        (HERE/dest).write_text(text,encoding='utf-8')


if __name__=='__main__':build_scoped_readers()
