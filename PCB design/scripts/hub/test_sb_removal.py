"""Real-export acceptance tests. Run BEFORE AFTER [BEFORE_LIVE AFTER_LIVE [BEFORE_SCH AFTER_SCH]].

Before board writes, run with BEFORE twice: component-count/removal checks must fail.
No expected-geometry generator is used: surviving records must equal the original bytes.
"""
import collections
import json
from pathlib import Path
import re
import sys
import unittest
import zipfile


TARGETS = {f'SB{k}' for k in range(1, 8)}
PCB = 'ad35aed4d7610fd2'
PATHS = sys.argv[1:]


def archive(path):
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None:
            raise AssertionError('Corrupt project archive')
        names = [n for n in z.namelist() if n.endswith('.epru')]
        if len(names) != 1:
            raise AssertionError('Expected exactly one project source')
        text = z.read(names[0]).decode('utf-8')
    docs = {}
    current = None
    decoder = json.JSONDecoder()
    for line in text.splitlines():
        left, sep, right = line.partition('||')
        if not sep or not right.startswith('{'):
            continue
        header = json.loads(left)
        body, end = decoder.raw_decode(right)
        if header['type'] == 'DOCHEAD':
            current = {'head': body, 'records': {}}
            if body['uuid'] in docs:
                raise AssertionError('Duplicate document UUID')
            docs[body['uuid']] = current
        elif current is not None:
            key = (header['type'], header['id'])
            if key in current['records']:
                raise AssertionError(f'Duplicate record: {key}')
            current['records'][key] = {'body': body, 'raw': right[:end]}
    if docs[PCB]['head']['docType'] != 'PCB':
        raise AssertionError('Wrong PCB document')
    return docs


def records(docs):
    return docs[PCB]['records']


def designators(recs):
    return {v['body']['value']: v['body']['parentId'] for (t, _), v in recs.items()
            if t == 'ATTR' and v['body'].get('key') == 'Designator'}


def owned(key, value, ids):
    kind, rid = key
    body = value['body']
    if kind in {'COMPONENT', 'WIRE'}:
        return rid in ids
    if body.get('parentId') in ids:
        return True
    if kind == 'LINE' and body.get('lineGroup') in ids:
        return True
    return kind == 'PAD_NET' and json.loads(rid)[1] in ids


def live(path):
    data = json.loads(Path(path).read_text(encoding='utf-8'))
    return json.loads(data) if isinstance(data, str) else data


@unittest.skipUnless(__name__ == '__main__', 'Run acceptance tests with explicit project archive paths')
class SubtractionAcceptance(unittest.TestCase):
    def test_01_exact_seven_removed_no_additions(self):
        self.assertEqual(sum(t == 'COMPONENT' for t, _ in B), 84)
        self.assertEqual(sum(t == 'COMPONENT' for t, _ in A), 77)
        self.assertEqual(set(BD) - set(AD), TARGETS)
        self.assertEqual(set(AD) - set(BD), set())
        self.assertFalse(any(re.fullmatch(r'SB\d+', d) for d in AD))

    def test_02_all_77_survivors_and_their_pad_nets_byte_identical(self):
        survivors = set(BD) - TARGETS
        self.assertEqual(len(survivors), 77)
        for d in sorted(survivors):
            self.assertEqual(AD.get(d), BD[d], d)
            old = {k: v['raw'] for k, v in B.items() if owned(k, v, {BD[d]})}
            new = {k: v['raw'] for k, v in A.items() if owned(k, v, {BD[d]})}
            self.assertEqual(new, old, d)

    def test_03_all_net_definitions_byte_identical(self):
        old = {k: v['raw'] for k, v in B.items() if k[0] == 'NET'}
        new = {k: v['raw'] for k, v in A.items() if k[0] == 'NET'}
        self.assertEqual(len(old), 75)  # 74 named nets plus the empty-net entry.
        self.assertEqual(new, old)

    def test_04_no_unapproved_pcb_record_changes(self):
        # This particular preflight found no bypass notes and NO routed copper.
        # Consequently only the seven components and their owned records may disappear.
        # ELE_PLACEHOLDER is a primitive-ID allocator counter, not a physical primitive.
        expected = {k: v['raw'] for k, v in B.items()
                    if k[0] != 'ELE_PLACEHOLDER' and not owned(k, v, SB_IDS)}
        self.assertEqual({k: v['raw'] for k, v in A.items() if k[0] != 'ELE_PLACEHOLDER'}, expected)
        def counters(recs):
            return collections.Counter((v['body']['dataType'], v['body']['max'])
                                       for (t, _), v in recs.items() if t == 'ELE_PLACEHOLDER')
        self.assertEqual(counters(A), counters(B) + collections.Counter({('COMPONENT', 7): 1, ('ATTR', 21): 1}))

    def test_05_all_other_documents_unchanged(self):
        self.assertEqual(set(AFTER), set(BEFORE))
        for uuid, doc in BEFORE.items():
            if uuid == '7e64f4e0d47bf001' and len(PATHS) == 6:
                old = doc['records']
                target_ids = {designators(old)[d] for d in TARGETS} | SCH_STUB_IDS
                expected = {k: v['raw'] for k, v in old.items()
                            if k[0] != 'ELE_PLACEHOLDER' and not owned(k, v, target_ids)}
                now = AFTER[uuid]['records']
                self.assertEqual({k: v['raw'] for k, v in now.items() if k[0] != 'ELE_PLACEHOLDER'}, expected)
                self.assertEqual(sum(k[0] == 'COMPONENT' for k in now), 78)  # 77 electrical parts + frame.
                self.assertEqual(sum(k[0] == 'WIRE' for k in now), 256)
                self.assertFalse(TARGETS & set(designators(now)))
            elif uuid != PCB:
                self.assertEqual(AFTER[uuid]['records'], doc['records'], uuid)

    def test_06_copper_pours_holes_and_outline_byte_identical(self):
        kinds = {'LINE', 'POLY', 'PAD', 'VIA', 'POUR', 'FILL', 'ARC', 'REGION'}
        old = {k: v['raw'] for k, v in B.items() if k[0] in kinds}
        new = {k: v['raw'] for k, v in A.items() if k[0] in kinds}
        self.assertEqual(new, old)
        copper = [(k, v['body']) for k, v in B.items()
                  if k[0] in kinds - {'PAD'} and
                  (v['body'].get('layerId') in {1, 2} or k[0] in {'VIA', 'POUR', 'FILL'})]
        self.assertEqual(copper, [], 'Baseline copper changed: requires new stub analysis')

    def test_07_no_sb_silk_or_bypass_note(self):
        for (kind, _), v in A.items():
            b = v['body']
            if kind in {'STRING', 'ATTR'} and b.get('layerId') in {3, 4}:
                text = b.get('text', b.get('value', ''))
                self.assertNotRegex(str(text), r'(?i)\bbypass\b|\bSB\d+\b')

    def test_08_hop_resistors_and_port_led_pins(self):
        def pads(recs, cid):
            return {json.loads(rid)[2]: v['body']['padNet'] for (t, rid), v in recs.items()
                    if t == 'PAD_NET' and json.loads(rid)[1] == cid}
        for k in range(1, 8):
            des = f'R{60+k}'
            self.assertIn(des, AD)
            self.assertEqual(pads(A, AD[des]), pads(B, BD[des]))
            self.assertEqual(set(pads(A, AD[des]).values()), {f'S{k}_DOUT', f'S{k+1}_DIN'})
        for k in range(1, 9):
            des = f'J{9+k}'
            self.assertIn(des, AD)
            now, old = pads(A, AD[des]), pads(B, BD[des])
            self.assertEqual(now, old)
            self.assertEqual(now['7'], f'S{k}_DIN')
            if k < 8:
                self.assertEqual(now['8'], f'S{k}_DOUT')

    def test_09_baseline_targets_are_c492401_two_pin_100_mil_headers(self):
        fps = set()
        for d in TARGETS:
            attrs = {v['body']['key']: v['body']['value'] for (t, _), v in B.items()
                     if t == 'ATTR' and v['body'].get('parentId') == BD[d]}
            fps.add(attrs['Footprint'])
            fp = BEFORE[attrs['Footprint']]['records']
            pads = [v['body'] for (t, _), v in fp.items() if t == 'PAD']
            self.assertEqual(len(pads), 2)
            self.assertEqual(sorted(p['num'] for p in pads), ['1', '2'])
            self.assertAlmostEqual(abs(pads[0]['centerX'] - pads[1]['centerX']), 100)
            self.assertEqual(pads[0]['centerY'], pads[1]['centerY'])
        matches = [v['body']['attributes'] for doc in BEFORE.values()
                   if doc['head']['docType'] == 'DEVICE' for (t, _), v in doc['records'].items()
                   if t == 'META' and v['body'].get('attributes', {}).get('Supplier Part') == 'C492401']
        self.assertEqual(len(matches), 1)
        self.assertEqual(fps, {matches[0]['Footprint']})

    @unittest.skipUnless(len(PATHS) >= 4, 'Live snapshots not supplied')
    def test_10_live_geometry_nets_and_silk_match(self):
        old, new = live(PATHS[2]), live(PATHS[3])
        for data in (old, new):
            self.assertEqual(data['project'], 'WRO_HUB')
            self.assertEqual(data['pcb'], PCB)
        self.assertEqual(len(old['components']), 84)
        self.assertEqual(len(new['components']), 77)
        self.assertEqual([c for c in old['components'] if c['designator'] not in TARGETS], new['components'])
        self.assertEqual(new['nets'], old['nets'])
        self.assertEqual(new['strings'], old['strings'])
        self.assertEqual(old['lineCount'], 0)
        self.assertEqual(new['lineCount'], 0)
        self.assertFalse(any(re.fullmatch(r'SB\d+', str(a['value'])) for a in new['attributes']))

    @unittest.skipUnless(len(PATHS) >= 4, 'Live snapshots not supplied')
    def test_11_no_new_drc_findings_and_only_sb_pad_findings_removed(self):
        def leaves(node):
            if isinstance(node, list):
                return [leaf for item in node for leaf in leaves(item)]
            if isinstance(node, dict):
                if 'list' in node:
                    return leaves(node['list'])
                if 'errorType' in node:
                    return [node]
            return []

        def stable(value):
            if isinstance(value, dict):
                return {k: stable(v) for k, v in value.items()
                        if k not in {'globalIndex', 'visible', 'parentId'}}
            if isinstance(value, list):
                return [stable(v) for v in value]
            return value

        old, new = live(PATHS[2]), live(PATHS[3])
        before_errors, after_errors = leaves(old['drc']), leaves(new['drc'])
        self.assertEqual(len(before_errors), 270)
        sb_pads = {p['id'] for c in old['components'] if c['designator'] in TARGETS for p in c['pads']}
        expected = [e for e in before_errors if not (set(e.get('objs', [])) & sb_pads)]
        self.assertEqual(len(before_errors) - len(expected), 14)
        self.assertEqual(len(after_errors), 256)
        self.assertEqual(collections.Counter(json.dumps(stable(e), sort_keys=True) for e in after_errors),
                         collections.Counter(json.dumps(stable(e), sort_keys=True) for e in expected))

    @unittest.skipUnless(len(PATHS) == 6, 'Schematic snapshots not supplied')
    def test_12_surviving_schematic_symbols_wires_and_pin_memberships_unchanged(self):
        old, new = live(PATHS[4]), live(PATHS[5])
        self.assertEqual(len(old['components']), 85)  # 84 electrical parts + title frame.
        self.assertEqual(len(new['components']), 78)
        self.assertEqual([c for c in old['components'] if c.get('designator') not in TARGETS], new['components'])
        self.assertEqual([w for w in old['wires'] if w['id'] not in SCH_STUB_IDS], new['wires'])
        self.assertEqual(len(old['wires']), 270)
        self.assertEqual(len(new['wires']), 256)
        self.assertEqual(len(old['nets']), 74)
        self.assertEqual(new['nets'], old['nets'])
        self.assertEqual([p for p in old['pinNets'] if p['pin'].split('.')[0] not in TARGETS], new['pinNets'])
        self.assertEqual(old['erc'], True)
        self.assertEqual(new['erc'], True)

    @unittest.skipUnless(len(PATHS) == 6, 'Schematic snapshots not supplied')
    def test_13_schematic_stub_allowlist_is_exclusive_to_sb_pins(self):
        old = live(PATHS[4])
        stubs = [w for w in old['wires'] if w['id'] in SCH_STUB_IDS]
        self.assertEqual(len(stubs), 14)
        def on(point, wire):
            x, y = point
            a, b, c, d = wire['line']
            return min(a, c) <= x <= max(a, c) and min(b, d) <= y <= max(b, d) and (x-a)*(d-b) == (y-b)*(c-a)
        for stub in stubs:
            self.assertEqual(len(stub['line']), 4)
            a, b, c, d = stub['line']
            self.assertEqual(b, d)
            self.assertEqual(abs(a-c), 30)
            attached = [(comp, pin) for comp in old['components'] for pin in comp['pins']
                        if on((pin['x'], pin['y']), stub)]
            self.assertEqual(len(attached), 1)
            comp, pin = attached[0]
            self.assertIn(comp['designator'], TARGETS)
            k = comp['designator'][2:]
            self.assertEqual(stub['net'], f'S{k}_' + ('DIN' if pin['number'] == '1' else 'DOUT'))
            for w in old['wires']:
                if w['id'] == stub['id']:
                    continue
                self.assertEqual(len(w['line']), 4)
                x, y, z, q = w['line']
                # Existing wires are all orthogonal; test overlap and crossings inclusively.
                self.assertTrue(x == z or y == q)
                cross = (min(x, z) <= max(a, c) and max(x, z) >= min(a, c) and
                         min(y, q) <= b <= max(y, q))
                self.assertFalse(cross, f"SB stub {stub['id']} intersects {w['id']}")


if __name__ == '__main__':
    PATHS = sys.argv[1:]
    if len(PATHS) not in {2, 4, 6}:
        sys.exit(__doc__)
    BEFORE, AFTER = (archive(p) for p in PATHS[:2])
    B, A = records(BEFORE), records(AFTER)
    BD, AD = designators(B), designators(A)
    SB_IDS = {BD[d] for d in TARGETS}
    SCH_STUB_IDS = set()
    if len(PATHS) == 6:
        sch = live(PATHS[4])
        pins = {(p['x'], p['y']) for c in sch['components'] if c.get('designator') in TARGETS for p in c['pins']}
        SCH_STUB_IDS = {w['id'] for w in sch['wires']
                        if tuple(w['line'][:2]) in pins or tuple(w['line'][-2:]) in pins}
    unittest.main(argv=[sys.argv[0]], verbosity=2)
