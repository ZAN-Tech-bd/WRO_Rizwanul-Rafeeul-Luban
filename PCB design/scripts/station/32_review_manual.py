"""Read-only review / offline pour generation from 31_snapshot.js output.

Usage: python 32_review_manual.py SNAPSHOT OUTPUT_DIR [--generate] [--baseline=SNAPSHOT]
Generated 42_fill.js is the ONLY writing script; it aborts if the source geometry changed.
"""
import json
import math
from pathlib import Path
import sys

from shapely.geometry import LineString, Point, Polygon
from shapely.ops import unary_union

import manual_model


def read(path):
    d = json.loads(Path(path).read_text(encoding='utf-8'))
    return json.loads(d) if isinstance(d, str) else d


def main():
    source, output = Path(sys.argv[1]), Path(sys.argv[2])
    output.mkdir(parents=True, exist_ok=True)
    (output / 'data').mkdir(exist_ok=True)
    s = read(source)
    pads, holes, board = manual_model.configure(s)
    import check_eda_copper as CE
    import route_check as RC
    import pour_fill as PF
    import check_fills as CF

    copper_path = output / (source.stem + '_copper.json')
    copper_path.write_text(json.dumps(s['copper']), encoding='utf-8')
    routes, nonet = CE.load(copper_path)
    for p in pads:
        if p.net:
            routes.setdefault(p.net, dict(segs=[], vias=[]))
    checks = RC.check(routes, 6.0)
    islands = CE.islands(routes, pads)
    result = dict(board_mm=[round(v * 0.0254, 3) for v in board.bounds[2:]],
                  components=len(s['components']), pads=len(pads), mounting_holes=len(holes),
                  routes=sum(len(r['segs']) for r in routes.values()),
                  vias=sum(len(r['vias']) for r in routes.values()),
                  unnamed_copper=bool(nonet['segs'] or nonet['vias'] or nonet.get('arcs')),
                  clearance_below_6mil=checks[0], edge_violations=checks[1], hole_violations=checks[2],
                  untouched_pads=checks[4], hole_to_hole=RC.check.hole2hole,
                  thin_power_segments=RC.check.thin_power,
                  jlc_hole_clearance=CE.hole_rules(routes, pads, holes),
                  signal_islands={n:len(g) for n,g in islands.items() if n != 'GND'},
                  power_width_mm={n:sorted(set(round(t[5]*0.0254, 3) for t in r['segs']))
                                  for n,r in routes.items() if n in ('5V','3V3')})
    result['pad_edge_min_mil'] = round(min(board.boundary.distance(Polygon(p.poly))
                                         if board.covers(Polygon(p.poly)) else -1 for p in pads), 3)
    result['annular_ring_min_mil'] = round(min((min(p.w,p.h)-p.hole)/2 for p in pads if p.tht), 3)
    result['pad_to_pad_below_6mil'] = [(str(a),str(b),round(Polygon(a.poly).distance(Polygon(b.poly)),3))
        for i,a in enumerate(pads) for b in pads[i+1:] if (a.net != b.net or not a.net)
        and set(a.layers)&set(b.layers) and Polygon(a.poly).distance(Polygon(b.poly)) < 6]
    expected = {
        '5V':{'J1.1','J2.1','U2.3','J4.1','Q1.3'},
        'GND':{'J1.2','J2.2','J3.2','U2.2','J4.2','BZ1.2','R3.2'},
        '3V3':{'J1.3','J3.1'}, 'DHT':{'J1.4','J3.3'}, 'MQ':{'J1.5','R2.2','R3.1'},
        'MQ_AO':{'J4.3','R2.1'}, 'PROX':{'J1.6','D1.2'}, 'IR_OUT':{'U2.1','D1.1','R1.1'},
        'Q_BASE':{'R1.2','Q1.2'}, 'BUZ_P':{'Q1.1','BZ1.1'}, 'DIN':{'J1.7','J2.3'}, 'DOUT':{'J1.8','J2.4'},
    }
    actual = {n:{'%s.%s'%(p.des,p.num) for p in pads if p.net==n} for n in {p.net for p in pads if p.net}}
    result['netlist_matches_spec'] = actual == expected
    result['no_connect_pads'] = ['%s.%s'%(p.des,p.num) for p in pads if not p.net]
    # Width/current calculations are not a substitute for a thermal/current review.
    fill_polys = {0:[],1:[]}
    for f in s['fills']:
        if f['net']=='GND' and f['layer'] in (1,2):
            rr = CF.rings_of(f['source'])
            if len(rr)!=1:
                raise ValueError('Read-back fill lost/has holes')
            g=Polygon(rr[0])
            if not g.is_valid:
                raise ValueError('Invalid read-back fill')
            fill_polys[f['layer']-1].append(g)
    if s['fills']:
        fp=output/(source.stem+'_fills.json')
        fp.write_text(json.dumps([[f['layer'],f['source']] for f in s['fills'] if f['net']=='GND']),encoding='utf-8')
        CF.HERE=str(output)
        old=sys.argv
        sys.argv=['check_fills.py',str(copper_path),str(fp),'--plan']
        CF.main()
        sys.argv=old
        # Physically joined copper across both layers, including every GND pad.
        nodes=[(L,g,'fill') for L in (0,1) for g in fill_polys[L]]
        nodes += [('both' if p.tht else p.layers[0],Polygon(p.poly),'%s.%s'%(p.des,p.num)) for p in pads if p.net=='GND']
        nodes += [(t[0],PF.seg_shape(t),'track') for t in routes['GND']['segs']]
        nodes += [('both',Point(v[0],v[1]).buffer(v[3]/2,quad_segs=32),'via') for v in routes['GND']['vias']]
        parents=list(range(len(nodes)))
        def root(i):
            while parents[i]!=i:
                parents[i]=parents[parents[i]]
                i=parents[i]
            return i
        for i,(L,a,_) in enumerate(nodes):
            for j,(K,b,_) in enumerate(nodes[:i]):
                if (L=='both' or K=='both' or L==K) and a.intersection(b).area>0.0001:
                    parents[root(i)]=root(j)
        groups={}
        for i,(_,_,lab) in enumerate(nodes):
            groups.setdefault(root(i),[]).append(lab)
        result['ground_copper_islands']=len(groups)
        result['ground_pad_groups']=[[lab for lab in group if lab not in ('fill','track','via')] for group in groups.values()]
        result['bottom_ground_area_mm2']=round(unary_union(fill_polys[1]).area*0.0254**2,2)
    baseline=next((v.split('=',1)[1] for v in sys.argv if v.startswith('--baseline=')),None)
    if baseline:
        before=read(baseline)
        # A stable ID/geometric comparison, independent of object enumeration order.
        def stable(d):
            return json.dumps(d,sort_keys=True,separators=(',',':'))
        result['user_copper_unchanged']=stable(s['copper'])==stable(before['copper'])
        result['user_components_unchanged']=sorted(map(stable,s['components']))==sorted(map(stable,before['components']))
        result['user_pads_unchanged']=sorted(map(stable,s['pads']))==sorted(map(stable,before['pads']))
        result['outline_and_silk_lines_unchanged']=sorted(map(stable,s['polylines']))==sorted(map(stable,before['polylines']))
    print(json.dumps(result,indent=2))
    (output/(source.stem+'_review.json')).write_text(json.dumps(result,indent=2),encoding='utf-8')
    if '--generate' in sys.argv:
        if not result['netlist_matches_spec'] or result['unnamed_copper'] or s['fills']:
            raise ValueError('Unexpected nets/existing copper fills: do not generate automatically')
        if any((checks[0],checks[1],checks[2],checks[4],result['jlc_hole_clearance'],result['pad_to_pad_below_6mil'])):
            raise ValueError('Fix copper violations before adding a pour')
        if any(len(groups)!=1 for net,groups in islands.items() if net!='GND'):
            raise ValueError('Signal net not physically connected')
        PF.HERE=str(output)
        # Fail on disconnected ground before writing any live editor primitive.
        old=sys.argv
        sys.argv=['pour_fill.py',str(copper_path),'--dry']
        lonely=PF.main()
        if lonely:
            raise ValueError('Bottom-only pour cannot reach every ground pad: %r'%lonely)
        sys.argv=['pour_fill.py',str(copper_path)]
        PF.main()
        sys.argv=old
        generated=output/'42_fill.js'
        snapshot_code=Path(__file__).with_name('31_snapshot.js').read_text(encoding='utf-8')
        # Re-read ALL source geometry immediately before writing, not merely the project name.
        guard='''// Snapshot-locked: preserve the user's exact current design; abort on any intervening edit.
const current = await (async () => {\n'''+snapshot_code+'''\n})();
const expected = '''+json.dumps(s,separators=(',',':'))+''';
const canonical = (x) => JSON.stringify(sortKeys(x));
function sortKeys(x) { if(Array.isArray(x)) return x.map(sortKeys); if(x && typeof x==='object')
  return Object.fromEntries(Object.keys(x).sort().map(k=>[k,sortKeys(x[k])])); return x; }
const geometry = (s) => ({projectUuid:s.projectUuid,tabId:s.tabId,copper:s.copper,
  components:s.components.slice().sort((a,b)=>a.id.localeCompare(b.id)),
  pads:s.pads.slice().sort((a,b)=>a.id.localeCompare(b.id)),
  polylines:s.polylines.slice().sort((a,b)=>a.id.localeCompare(b.id)),regions:s.regions});
if(canonical(geometry(current))!==canonical(geometry(expected))) throw new Error('Board changed: regenerate pour from a fresh snapshot');
if(current.fills.some(f=>f.net!=='GND'||f.layer!==2)||current.pours.length) throw new Error('Unexpected existing fills/pours');
'''
        generated.write_text(guard+generated.read_text(encoding='utf-8'),encoding='utf-8')
        print('Generated guarded bottom-GND writer:',generated)


if __name__ == '__main__':
    main()
