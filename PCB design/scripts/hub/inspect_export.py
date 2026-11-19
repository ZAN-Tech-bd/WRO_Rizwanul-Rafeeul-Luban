"""Read an exported WRO_HUB .epro2 (READ-ONLY) and report what EasyEDA actually serialized for one PCB component:
its COMPONENT record, Footprint / Device / supplier attributes, PAD_NET rows, the referenced footprint document's pads
(hole type / size / rotation in mm) and graphics, the board outline, and every footprint document without pads.

Run: python inspect_export.py <file.epro2> [designator=J3]
"""
import json
import sys
import zipfile

MIL = 0.0254


def documents(path):
    with zipfile.ZipFile(path) as z:
        name = next(n for n in z.namelist() if n.endswith('.epru'))
        text = z.read(name).decode('utf-8')
    docs, head = [], None
    decoder = json.JSONDecoder()
    for line in text.splitlines():
        a, sep, b = line.partition('||')
        if not sep or not b.startswith('{'):
            continue
        a, b = json.loads(a), decoder.raw_decode(b)[0]
        if a['type'] == 'DOCHEAD':
            head = b
            docs.append((head, []))
        elif docs:
            docs[-1][1].append((a, b))
    return docs


def pad_mm(p):
    h, d = p.get('hole') or {}, p.get('defaultPad') or {}
    return {'num': p['num'], 'net': p.get('netName'), 'xy_mm': [round(p['centerX'] * MIL, 3), round(p['centerY'] * MIL, 3)],
            'padAngle': p.get('padAngle'), 'pad': [d.get('padType'), round(d.get('width', 0) * MIL, 3), round(d.get('height', 0) * MIL, 3)],
            'hole': [h.get('holeType'), round(h.get('width', 0) * MIL, 3), round(h.get('height', 0) * MIL, 3)],
            'relativeAngle': p.get('relativeAngle'), 'plated': p.get('plated')}


def inspect(path, des='J3'):
    docs = documents(path)
    pcb = [(h, r) for h, r in docs if h.get('docType') == 'PCB']
    fps = {h['uuid']: r for h, r in docs if h.get('docType') == 'FOOTPRINT'}
    report = {'file': str(path), 'pcbDocs': len(pcb), 'footprints': len(fps),
              'footprintsWithoutPads': [u for u, r in fps.items() if not any(a['type'] == 'PAD' for a, _ in r)]}
    for head, recs in pcb:
        comps = {a['id']: b for a, b in recs if a['type'] == 'COMPONENT'}
        attrs = {}
        for a, b in recs:
            if a['type'] == 'ATTR' and b.get('parentId') in comps:
                attrs.setdefault(b['parentId'], {})[b['key']] = b['value']
        report['components'] = len(comps)
        cid = next((c for c, at in attrs.items() if at.get('Designator') == des), None)
        if not cid:
            report['error'] = f'{des} not found'
            continue
        at = attrs[cid]
        report[des] = {'component': comps[cid], 'attrs': at,
                       'padNets': sorted((json.loads(a['id'])[2], json.loads(a['id'])[3], b['padNet'])
                                         for a, b in recs if a['type'] == 'PAD_NET' and json.loads(a['id'])[1] == cid)}
        fp = fps.get(at.get('Footprint'))
        report[des]['footprint'] = None if fp is None else {
            'uuid': at.get('Footprint'),
            'pads': [dict(pad_mm(b), id=a['id']) for a, b in fp if a['type'] == 'PAD'],
            'polys': [(b.get('layerId'), len(b.get('path', []))) for a, b in fp if a['type'] == 'POLY'],
            'records': len(fp)}
        report['outline'] = [b.get('path') for a, b in recs if a['type'] == 'POLY' and b.get('layerId') == 11]
    return report


if __name__ == '__main__':
    print(json.dumps(inspect(sys.argv[1], *(sys.argv[2:3] or ['J3'])), indent=1))
