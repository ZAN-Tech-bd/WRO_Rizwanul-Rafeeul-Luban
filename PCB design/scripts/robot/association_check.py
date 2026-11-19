"""READ-ONLY to EasyEDA: decode a guarded export and compare every native body.

Never saves, imports, restores, or modifies an EasyEDA document. Creates new local
evidence only, refusing to overwrite the archive or report.
"""
import argparse
import base64
from collections import Counter
import hashlib
import json
from pathlib import Path
import zipfile

import gnd_edit


def differences(a, b, path=''):
    """Describe exact JSON-value changes without ignoring any field."""
    if type(a) is not type(b):
        return [dict(path=path, before=a, after=b)]
    if isinstance(a, dict):
        rows = []
        for key in sorted(a.keys() | b.keys()):
            p = path + '/' + str(key)
            if key not in a:
                rows.append(dict(path=p, added=b[key]))
            elif key not in b:
                rows.append(dict(path=p, removed=a[key]))
            else:
                rows.extend(differences(a[key], b[key], p))
        return rows
    if a != b:
        return [dict(path=path, before=a, after=b)]
    return []


def archive_members(path):
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None, 'ZIP integrity failure'
        assert len(z.namelist()) == len(set(z.namelist())), 'Duplicate ZIP member'
        return {name: z.read(name) for name in z.namelist()}


def compare(before, after):
    a, b = gnd_edit.read_archive(before), gnd_edit.read_archive(after)
    rows = []
    unchanged = Counter()
    raw_only = []
    heads = []
    for doc in sorted(a.keys() | b.keys()):
        if doc not in a or doc not in b:
            rows.append(dict(document=doc, kind='document-added' if doc in b else 'document-removed'))
            continue
        if a[doc]['head'] != b[doc]['head']:
            heads.append(dict(document=doc, changes=differences(a[doc]['head'], b[doc]['head'])))
        ar, br = a[doc]['records'], b[doc]['records']
        for key in sorted(ar.keys() | br.keys()):
            common = dict(document=doc, type=key[0], id=key[1])
            if key not in ar:
                rows.append(dict(common, kind='added', after=br[key]['body']))
            elif key not in br:
                rows.append(dict(common, kind='removed', before=ar[key]['body']))
            elif ar[key]['body'] != br[key]['body']:
                rows.append(dict(common, kind='modified', changes=differences(ar[key]['body'], br[key]['body'])))
            else:
                unchanged[key[0]] += 1
                if ar[key]['raw'] != br[key]['raw']:
                    raw_only.append(common)
    am, bm = archive_members(before), archive_members(after)
    member_changes = []
    for name in sorted(am.keys() | bm.keys()):
        if name in am and name in bm and am[name] == bm[name]:
            continue
        r = dict(name=name, beforeSha256=hashlib.sha256(am[name]).hexdigest() if name in am else None,
                 afterSha256=hashlib.sha256(bm[name]).hexdigest() if name in bm else None)
        if name.endswith('.json') and name in am and name in bm:
            r['changes'] = differences(json.loads(am[name]), json.loads(bm[name]))
        member_changes.append(r)
    pcb = b[gnd_edit.PCB]['records']
    xt = {}
    for uuid in ('eac10ca20e6778ba', '9e78c3c3ca74ad42', '9197bb2e6c38da76'):
        xt[uuid] = [dict(id=i, num=r['body'].get('num'), hole=r['body'].get('hole'),
                         pad=r['body'].get('defaultPad'))
                    for (t, i), r in b[uuid]['records'].items() if t == 'PAD']
    return dict(before=str(before), after=str(after), documentsBefore=len(a), documentsAfter=len(b),
                recordChanges=rows, documentHeaderChanges=heads, serializationOnlyChanges=raw_only,
                unchangedBodiesByType=dict(unchanged), archiveMemberChanges=member_changes,
                pcbRecordCounts=dict(Counter(t for t, _ in pcb)), xtFootprintPads=xt,
                allNativeBodiesAndDocumentHeadersEqual=not rows and not heads)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--response', type=Path)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    args = p.parse_args()
    assert not args.report.exists(), 'Refusing to overwrite report'
    if args.response:
        assert not args.after.exists(), 'Refusing to overwrite export'
        value = json.loads(args.response.read_text(encoding='utf-8-sig'))
        if isinstance(value, str):
            value = json.loads(value)
        assert value['project'] == gnd_edit.PROJECT, 'Wrong export project'
        blob = base64.b64decode(value['b64'], validate=True)
        assert len(blob) == value['size']
        with args.after.open('xb') as f:
            f.write(blob)
    report = compare(args.before, args.after)
    report['sha256'] = {str(x): hashlib.sha256(x.read_bytes()).hexdigest() for x in (args.before, args.after)}
    with args.report.open('x', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
        f.write('\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('xtFootprintPads',)}, indent=2))
    print('XT footprint pads:', json.dumps(report['xtFootprintPads'], indent=2))


if __name__ == '__main__':
    main()
