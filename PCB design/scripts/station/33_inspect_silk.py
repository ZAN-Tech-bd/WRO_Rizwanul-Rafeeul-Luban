"""READ-ONLY: list the station PCB's silk strings and designator overlaps from a saved export."""
import json
import sys
import zipfile
from pathlib import Path

ARCHIVE = Path(__file__).resolve().parents[2] / 'station/station_silkfix_base_20261028.epro2'
PCB = 'fdcb20516f125209'


def main():
    z = zipfile.ZipFile(ARCHIVE)
    assert z.testzip() is None
    text = z.read(next(n for n in z.namelist() if n.endswith('.epru'))).decode('utf-8')
    dec = json.JSONDecoder()
    cur, docs, strings, attrs = None, 0, [], []
    for line in text.splitlines():
        left, sep, right = line.partition('||')
        if not sep or not right.startswith('{'):
            continue
        h = json.loads(left)
        b, _ = dec.raw_decode(right)
        if h['type'] == 'DOCHEAD':
            cur = b['uuid']
            docs += 1
        elif cur == PCB:
            if h['type'] == 'STRING':
                strings.append(dict(id=h['id'], text=b.get('text'), x=round(b.get('x', 0), 1),
                                    y=round(b.get('y', 0), 1), size=b.get('fontSize'),
                                    layer=b.get('layerId')))
            elif h['type'] == 'ATTR' and b.get('key') == 'Designator':
                attrs.append(dict(id=h['id'], parent=b.get('parentId'), value=b.get('value'),
                                  x=round(b.get('x', 0), 1), y=round(b.get('y', 0), 1)))
    print(json.dumps(dict(documents=docs, strings=strings, designators=attrs), indent=1))


if __name__ == '__main__':
    main()
