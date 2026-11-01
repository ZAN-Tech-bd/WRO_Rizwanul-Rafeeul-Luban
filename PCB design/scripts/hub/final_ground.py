"""Native-export-locked hub GND regeneration and independent exact-geometry acceptance.

No routing/placement/footprint edits. Dimensions are mil, read from native saved footprint records.
Normal fill clearance 12 mil; left area 40 mil; isolated source rails 50 mil. These apply to NEW
fill copper, not to the user's existing copper. JLC 1 oz baseline, not a fabrication release.
"""
import collections
from functools import lru_cache
import json
import math
from pathlib import Path
import sys

from shapely import affinity
from shapely import make_valid
from shapely.geometry import Point, LineString, Polygon, MultiPolygon, box
from shapely.ops import unary_union
from shapely.strtree import STRtree

from test_sb_removal import archive, live, PCB

HERE=Path(__file__).resolve().parent
DATA=HERE/'data/final_review_20261027'
BEFORE=HERE.parents[1]/'hub/WRO_HUB_before_ground_write_20261027-1647.epro2'
SOURCE_NETS={'BAT+','BAT-','DC+','DC-','V12'}
NORMAL,LEFT,SOURCE=12.0,40.0,50.0
EDGE,HOLE_CLEAR,MIN_NECK=16.0,20.0,12.0
QS=64
VIA_MOVES={'48a3854320122408':(1055,1170),'235a61808f8c555a':(375,2710),'a53c0f763e0bbf2d':(2945,3050)}


def from_source(src):
    if src and isinstance(src[0],list):
        if len(src)!=1:raise ValueError('Multiple rings not allowed in stored fill')
        src=src[0]
    if any(isinstance(x,str) and x!='L' for x in src):raise ValueError('Unsupported polygon command')
    nums=[x for x in src if not isinstance(x,str)]
    p=Polygon(list(zip(nums[::2],nums[1::2])))
    if not p.is_valid:raise ValueError('Invalid polygon')
    return p


def transform(shape,x,y,angle):
    return affinity.translate(affinity.rotate(shape,angle,origin=(0,0)),x,y)


def pad_shape(p):
    d=p['defaultPad'];w,h=d['width'],d['height'];t=d['padType']
    if t=='RECT':return box(-w/2,-h/2,w/2,h/2)
    if t=='ELLIPSE':return affinity.scale(Point(0,0).buffer(1,quad_segs=QS),w/2,h/2)
    if t=='OVAL':
        if abs(w-h)<1e-8:return Point(0,0).buffer(w/2,quad_segs=QS)
        pts=[(-(w-h)/2,0),((w-h)/2,0)] if w>h else [(0,-(h-w)/2),(0,(h-w)/2)]
        return LineString(pts).buffer(min(w,h)/2,quad_segs=QS)
    raise ValueError('Unsupported pad shape '+t)


def clearance(net,shape):
    if net in SOURCE_NETS:return SOURCE
    return LEFT if shape.bounds[0]<1000 else NORMAL


@lru_cache(maxsize=1)
def load_model():
    docs=archive(BEFORE);recs=docs[PCB]['records'];components={rid:r['body'] for (t,rid),r in recs.items() if t=='COMPONENT'}
    attrs={cid:{} for cid in components}
    for (t,_),r in recs.items():
        b=r['body']
        if t=='ATTR' and b.get('parentId') in attrs:attrs[b['parentId']][b['key']]=b['value']
    nets={(json.loads(rid)[1],json.loads(rid)[3]):r['body']['padNet'] for (t,rid),r in recs.items() if t=='PAD_NET'}
    pads=[];holes=[]
    def add(p,des,rid,cx=0,cy=0,rot=0,net=''):
        local=Point(p['centerX'],p['centerY']);center=transform(local,cx,cy,rot)
        angle=rot+p.get('padAngle',0)
        copper=transform(pad_shape(p),center.x,center.y,angle)
        hd=p.get('hole') or {};hole=None
        if hd.get('holeType')=='ROUND':hole=Point(0,0).buffer(hd['width']/2,quad_segs=QS)
        elif hd.get('holeType')=='SLOT':
            length,width=hd['width'],hd['height']
            hole=LineString([(-(length-width)/2,0),((length-width)/2,0)]).buffer(width/2,quad_segs=QS)
        if hole is not None:
            hole=transform(hole,center.x,center.y,angle+p.get('relativeAngle',0));holes.append({'des':des,'num':p['num'],'shape':hole,'net':net})
        pad={'des':des,'num':p['num'],'id':rid,'net':net,'shape':copper,'hole':hole,'center':(center.x,center.y),
             'layers':(1,2) if p['layerId']==12 else (p['layerId'],),'native':p}
        pads.append(pad)
    for cid,c in components.items():
        at=attrs[cid];fp=docs[at['Footprint']]['records']
        for (t,rid),r in fp.items():
            if t=='PAD':add(r['body'],at['Designator'],cid+rid,c['x'],c['y'],c['angle'],nets.get((cid,rid),''))
    for (t,rid),r in recs.items():
        if t=='PAD':add(r['body'],'MH',rid,net=r['body'].get('netName',''))
    items=[]
    for p in pads:items.append({'kind':'pad','id':p['id'],'net':p['net'],'layers':p['layers'],'shape':p['shape'],'label':p['des']+'.'+p['num']})
    for (t,rid),r in recs.items():
        b=r['body']
        if t=='LINE' and b.get('layerId') in (1,2):
            a=(b['startX'],b['startY']);z=(b['endX'],b['endY']);g=Point(a) if a==z else LineString([a,z])
            items.append({'kind':'track','id':rid,'net':b['netName'],'layers':(b['layerId'],),'shape':g.buffer(b['width']/2,quad_segs=QS),'label':b['netName']+' track '+rid})
        elif t=='VIA':
            vx,vy=VIA_MOVES.get(rid,(b['centerX'],b['centerY']))
            items.append({'kind':'via','id':rid,'net':b['netName'],'layers':(1,2),'shape':Point(vx,vy).buffer(b['viaDiameter']/2,quad_segs=QS),'label':b['netName']+' via '+rid})
            holes.append({'des':'via','num':rid,'shape':Point(vx,vy).buffer(b['holeDiameter']/2,quad_segs=QS),'net':b['netName']})
        elif t=='FILL':raise ValueError('Source already contains fill; do not stack a new plane')
    outline=next(from_source(r['body']['path']) for (t,_),r in recs.items() if t=='POLY' and r['body'].get('layerId')==11)
    u=next(p for p in pads if p['des']=='U1A' and p['num']=='1');x,y=u['center']
    antenna=box(x-60,y-175,x+1060,outline.bounds[3]+100)
    current=live(DATA/'live_current.json');expected={(c['des'],p['num']):p for c in current['comps'] for p in c['pads']}
    mismatch=[]
    for p in pads:
        if p['des']=='MH':continue
        q=expected.get((p['des'],p['num']))
        if not q or q['net']!=p['net'] or math.dist((q['x'],q['y']),p['center'])>.2:mismatch.append(p['des']+'.'+p['num'])
    return {'docs':docs,'records':recs,'components':components,'attrs':attrs,'pads':pads,'holes':holes,'items':items,
            'outline':outline,'antenna':antenna,'padNetMismatches':mismatch}


def pieces(poly,overlap=1.0):
    if poly.is_empty:return []
    if not isinstance(poly,Polygon):
        return [q for p in poly.geoms for q in pieces(p,overlap)] if hasattr(poly,'geoms') else []
    if not poly.interiors:return [poly] if poly.area>=4 else []
    xs=sorted({round(Polygon(r).representative_point().x,3) for r in poly.interiors})
    x0,y0,x1,y1=poly.bounds
    return [q for a,b in zip([x0-1]+xs,xs+[x1+1]) for q in pieces(poly.intersection(box(a-overlap,y0-1,b+overlap,y1+1)),overlap)]


def connectivity(fills,model):
    nodes=[]
    for layer,geoms in fills.items():nodes.extend((layer,p,'fill',str(i)) for i,p in enumerate(geoms))
    for item in model['items']:
        if item['net']=='GND':nodes.append((0 if len(item['layers'])==2 else item['layers'][0],item['shape'],item['kind'],item['label']))
    tree=STRtree([n[1] for n in nodes]);parent=list(range(len(nodes)))
    def root(i):
        while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
        return i
    for i,(la,shape,_,_) in enumerate(nodes):
        for j in tree.query(shape,predicate='intersects'):
            j=int(j)
            if j>i and (la==0 or nodes[j][0]==0 or la==nodes[j][0]):parent[root(i)]=root(j)
    groups=collections.defaultdict(list)
    for i,n in enumerate(nodes):groups[root(i)].append(i)
    main=max(groups.values(),key=lambda ids:sum(nodes[i][2]=='pad' for i in ids))
    mainset=set(main);disconnected=[nodes[i][3] for i in range(len(nodes)) if i not in mainset and nodes[i][2]!='fill']
    return {layer:[p for i,(l,p,kind,_) in enumerate(nodes) if l==layer and kind=='fill' and i in mainset] for layer in (1,2)},disconnected


def generate():
    m=load_model();assert not m['padNetMismatches'],m['padNetMismatches']
    x0,y0,x1,y1=m['outline'].bounds;left=box(x0-100,y0-100,1010,y1+100)
    allowed=m['outline'].buffer(-EDGE).difference(m['antenna'].buffer(8))
    fills={}
    for layer in (1,2):
        foreign=[i for i in m['items'] if i['net']!='GND' and layer in i['layers']]
        keep=[]
        for it in foreign:
            keep.append(it['shape'].buffer(SOURCE if it['net'] in SOURCE_NETS else NORMAL,quad_segs=24))
            if it['net'] not in SOURCE_NETS:keep.append(it['shape'].buffer(LEFT,quad_segs=24).intersection(left))
        blocked=unary_union(keep)
        area=allowed.difference(blocked)
        for p in m['pads']:
            if p['net']!='GND' or layer not in p['layers']:continue
            gap=14 if p['des'] in {'J3','J4','J5'} else 12
            ring=p['shape'].buffer(gap,quad_segs=24).difference(p['shape'])
            area=area.difference(ring)
            cx,cy=p['center'];a,b,c,d=p['shape'].bounds;reach=max(c-a,d-b)/2+gap+6
            sw=60 if p['des'] in {'J3','J4','J5'} else 20
            spokes=unary_union([box(cx-sw/2,cy-reach,cx+sw/2,cy+reach),box(cx-reach,cy-sw/2,cx+reach,cy+sw/2)])
            area=unary_union([area,spokes.intersection(allowed.difference(blocked))])
        area=area.buffer(-MIN_NECK/2,quad_segs=16).buffer(MIN_NECK/2,quad_segs=16).intersection(area)
        fills[layer]=[p for p in (area.geoms if hasattr(area,'geoms') else [area]) if isinstance(p,Polygon) and p.area>=400]
    fills,disconnected=connectivity(fills,m)
    output=[]
    for layer in (1,2):
        for poly in fills[layer]:
            for q in pieces(poly.simplify(.1,preserve_topology=True)):
                coords=[(round(x,3),round(y,3)) for x,y in q.exterior.coords]
                if coords[0]!=coords[-1]:coords.append(coords[0])
                src=[*coords[0],'L',*[v for p in coords[1:] for v in p]]
                output.append({'layer':layer,'source':src})
    plan={'sourceArchive':str(BEFORE),'normalClearanceMil':NORMAL,'leftClearanceMil':LEFT,'sourceClearanceMil':SOURCE,
          'pieces':output,'disconnectedGndBeforeSerialization':disconnected}
    (DATA/'ground_plan.json').write_text(json.dumps(plan,separators=(',',':')))
    repair_serialized_plan()
    report=verify_plan();(DATA/'ground_plan_check.json').write_text(json.dumps(report,indent=2))
    render(fills,m)
    print(json.dumps({k:v for k,v in report.items() if k not in {'foreignOverlaps','clearanceViolations'}},indent=1))
    if report['foreignOverlaps'] or report['clearanceViolations'] or report['disconnectedGnd']:raise AssertionError(report)


def load_plan():return json.loads((DATA/'ground_plan.json').read_text())


def repair_serialized_plan():
    """Split precision-rounded self-touches into valid hole-free pieces; never enlarge a keep-out."""
    plan=load_plan();fixed=[]
    for p in plan['pieces']:
        nums=[n for n in p['source'] if not isinstance(n,str)]
        poly=Polygon(list(zip(nums[::2],nums[1::2])))
        if poly.is_valid:fixed.append(p);continue
        for q in pieces(make_valid(poly)):
            coords=list(q.exterior.coords)
            src=[*coords[0],'L',*[n for pt in coords[1:] for n in pt]]
            assert from_source(src).is_valid
            fixed.append({'layer':p['layer'],'source':src})
    plan['pieces']=fixed
    (DATA/'ground_plan.json').write_text(json.dumps(plan,separators=(',',':')))


def verify_plan():
    plan=load_plan();m=load_model();fills={l:[from_source(p['source']) for p in plan['pieces'] if p['layer']==l] for l in (1,2)}
    overlaps=[];bad=[];minimum={'normal':1e9,'left':1e9,'source':1e9};bylayer={l:unary_union(fills[l]) for l in (1,2)}
    x0,y0,x1,y1=m['outline'].bounds;left=box(x0-1,y0-1,1000,y1+1)
    for it in m['items']:
        if it['net']=='GND':continue
        for l in it['layers']:
            g=bylayer[l];distance=g.distance(it['shape']);lim=SOURCE if it['net'] in SOURCE_NETS else NORMAL
            name='source' if it['net'] in SOURCE_NETS else 'normal';minimum[name]=min(minimum[name],distance)
            if g.intersection(it['shape']).area>.001:overlaps.append([l,it['label']])
            if distance<lim-.2:bad.append([l,it['label'],distance,lim])
            if it['net'] not in SOURCE_NETS:
                local=g.intersection(left)
                if not local.is_empty:
                    d=local.distance(it['shape']);minimum['left']=min(minimum['left'],d)
                    if d<LEFT-.2:bad.append([l,it['label'],'left',d,LEFT])
    connected,disconnected=connectivity(fills,m)
    floating=sum(len(fills[layer])-len(connected[layer]) for layer in (1,2))
    assert floating==0,f'{floating} fill pieces are outside the main GND network'
    edge=min(g.distance(m['outline'].exterior) for g in bylayer.values())
    ant=min(g.distance(m['antenna']) for g in bylayer.values())
    assert all(m['outline'].contains(g) for g in bylayer.values()),'Copper outside outline'
    assert edge>=EDGE-.2 and ant>=7.8,(edge,ant)
    return {'fillPieces':len(plan['pieces']),'layerCounts':{str(l):len(fills[l]) for l in fills},
            'foreignOverlaps':overlaps,'clearanceViolations':bad,'minimumClearanceMil':minimum,
            'edgeClearanceMil':edge,'antennaClearanceMil':ant,'disconnectedGnd':disconnected,
            'floatingFillPieces':floating}


def render(fills,m):
    from PIL import Image,ImageDraw,ImageFont
    x0,y0,x1,y1=m['outline'].bounds;s=.25;margin=30;w=int((x1-x0)*s);h=int((y1-y0)*s)
    im=Image.new('RGB',(w*2+margin*3,h+margin*2),'#15191f');d=ImageDraw.Draw(im)
    for l in (1,2):
        xy=lambda x,y:(margin+(l-1)*(w+margin)+(x-x0)*s,margin+(y1-y)*s)
        for p in fills[l]:d.polygon([xy(x,y) for x,y in p.exterior.coords],fill='#4d3630' if l==1 else '#253b60');[d.polygon([xy(x,y) for x,y in r.coords],fill='#15191f') for r in p.interiors]
        for it in m['items']:
            if l in it['layers']:d.polygon([xy(x,y) for x,y in it['shape'].exterior.coords],fill='#c1a143' if it['kind']=='pad' else '#b8644b' if l==1 else '#537fc4')
        d.line([xy(x,y) for x,y in m['outline'].exterior.coords],fill='white',width=2)
    im.save(DATA/'ground_preview.png')


def build_writer():
    m=load_model();r=m['records'];snap=live(DATA/'snapshot_current.json')
    lines=[dict(id=rid,net=v['body']['netName'],layer=v['body']['layerId'],x1=v['body']['startX'],y1=v['body']['startY'],
                x2=v['body']['endX'],y2=v['body']['endY'],width=v['body']['width'])
           for (t,rid),v in r.items() if t=='LINE' and v['body'].get('layerId') in (1,2)]
    vias=[dict(id=rid,net=v['body']['netName'],x=v['body']['centerX'],y=v['body']['centerY'],
               hole=v['body']['holeDiameter'],diameter=v['body']['viaDiameter']) for (t,rid),v in r.items() if t=='VIA']
    config=dict(components=snap['components'],lines=lines,vias=vias,viaMoves=VIA_MOVES,pieces=load_plan()['pieces'],
                legend=dict(id='49529d11268779e9',before='-  5V FROM BUCK  +',after='+  5V FROM BUCK  -'))
    template=(HERE/'66_final_ground.template.js').read_text(encoding='utf-8')
    (HERE/'66_final_ground.js').write_text(template.replace('__FINAL_GROUND_CONFIG__',json.dumps(config,separators=(',',':'))),encoding='utf-8')


if __name__=='__main__':
    generate()
    build_writer()
