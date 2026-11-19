"""READ-ONLY: decode a guarded station export and diff every native record vs a baseline."""
import argparse
import base64
import json
import zipfile
from pathlib import Path

PCB = 'fdcb20516f125209'
PROJECT = '2d2da5292f828ea674cc52b5833bd8a226b03f5595240d96a18801ebe8c8acea'


def read_archive(path):
    z = zipfile.ZipFile(path)
    assert z.testzip() is None
    text = z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
    dec = json.JSONDecoder()
    docs, cur = {}, None
    for line in text.splitlines():
        left, sep, right = line.partition('||')
        if not sep or not right.startswith('{'):
            continue
        h = json.loads(left)
        b, _ = dec.raw_decode(right)
        if h['type'] == 'DOCHEAD':
            cur = {'head': b, 'records': {}}
            docs[b['uuid']] = cur
        elif cur is not None:
            cur['records'][(h['type'], h['id'])] = b
    assert PCB in docs
    return docs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--response', type=Path)
    ap.add_argument('--before', type=Path, required=True)
    ap.add_argument('--after', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    if args.response:
        assert not args.after.exists()
        v = json.loads(args.response.read_text(encoding='utf-8-sig'))
        if isinstance(v, str):
            v = json.loads(v)
        assert v['project'] == PROJECT and v['pcb'] == PCB
        blob = base64.b64decode(v['b64'])
        assert len(blob) == v['size']
        args.after.write_bytes(blob)
    a, b = read_archive(args.before), read_archive(args.after)
    assert a.keys() == b.keys(), 'document set changed'
    rows = []
    from collections import Counter
    counts = Counter()
    for doc in a:
        ar, br = a[doc]['records'], b[doc]['records']
        for key in sorted(ar.keys() | br.keys()):
            if key not in ar:
                rows.append(dict(doc=doc, type=key[0], id=key[1], kind='added'))
            elif key not in br:
                rows.append(dict(doc=doc, type=key[0], id=key[1], kind='removed'))
            elif ar[key] != br[key]:
                rows.append(dict(doc=doc, type=key[0], id=key[1], kind='modified',
                                 before=ar[key], after=br[key]))
            else:
                counts[key[0]] += 1
    report = dict(changed=rows, unchanged=dict(counts), identical=len(rows) == 0)
    args.report.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(changedCount=len(rows),
                          changed=[{k: r[k] for k in ('doc', 'type', 'id', 'kind')} for r in rows],
                          unchangedTotals=dict(counts)), indent=2))


if __name__ == '__main__':
    main()
