"""H-005 acceptance: only FILL records were removed; everything else is byte-identical.

Run: python verify_gnd_fill_removal.py BEFORE.epro2 AFTER.epro2 [expected_fill_count]
expected_fill_count defaults to the dump file next to this script's data dir.
"""
import collections
import json
from pathlib import Path
import sys

from test_sb_removal import archive, live, PCB

HERE = Path(__file__).resolve().parent
DUMP = HERE / 'data/gnd_fill_backup_20261027/gnd_fills.json'


def verify(before_path, after_path, expected=None):
    if expected is None:
        expected = live(DUMP)['count']
    before, after = archive(before_path), archive(after_path)
    assert before.keys() == after.keys(), 'Project documents changed'
    b, a = before[PCB]['records'], after[PCB]['records']
    dump = live(DUMP)
    verified_ids = {f['id'] for f in dump['fills'] if f['net'] == 'GND' and f['layer'] in (1, 2)}
    removed_keys = b.keys() - a.keys()
    removed = collections.Counter(k[0] for k in b.keys() - a.keys())
    added = collections.Counter(k[0] for k in a.keys() - b.keys())
    assert removed['FILL'] == expected and set(removed) <= {'FILL', 'ELE_PLACEHOLDER'}, f'unexpected removals: {dict(removed)}'
    assert {rid for kind, rid in removed_keys if kind == 'FILL'} == verified_ids, 'Deleted fill identities differ from the backup'
    def allocators(recs):
        return collections.Counter((r['body'].get('dataType'), r['body'].get('max'))
                                   for (kind, _), r in recs.items() if kind == 'ELE_PLACEHOLDER')
    expected_allocators = collections.Counter({k: n for k, n in allocators(b).items() if k[0] != 'FILL'})
    assert allocators(a) == expected_allocators, 'Unexpected non-fill allocator changes'
    assert not any(kind == 'FILL' and r['body'].get('netName') == 'GND' and r['body'].get('layerId') in (1, 2)
                   for (kind, _), r in a.items()), 'Target GND fills remain'
    assert set(k[0] for k in added) <= {'ELE_PLACEHOLDER'}, f'unexpected additions: {dict(added)}'
    changed = [k for k in b.keys() & a.keys() if b[k]['raw'] != a[k]['raw']]
    assert all(k[0] == 'ELE_PLACEHOLDER' for k in changed), f'non-allocator records changed: {changed[:5]}'
    for uuid in before.keys() - {PCB}:
        assert before[uuid]['records'] == after[uuid]['records'], f'other document changed: {uuid}'
    report = {'fillsRemoved': removed['FILL'], 'fillAllocatorRecordsRemoved': removed['ELE_PLACEHOLDER'], 'addedRecords': dict(added),
              'changedRecords': len(changed), 'allOtherPcbRecordsByteIdentical': True,
              'allOtherProjectDocumentsUnchanged': True}
    (HERE / 'data/gnd_fill_backup_20261027/verification.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=1))


if __name__ == '__main__':
    args = [Path(x) if x.endswith('.epro2') else int(x) for x in sys.argv[1:]]
    verify(*args)
