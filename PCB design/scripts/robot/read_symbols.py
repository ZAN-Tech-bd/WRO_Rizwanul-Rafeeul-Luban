# List pin numbers and names of EasyEDA Pro symbol files (elibz zips from 09b_symbol_files.js).
# A PIN record is followed by ATTR records that carry its NAME and NUMBER (linked by the pin's id).
# Usage: python read_symbols.py data/sym_files.json
import base64
import io
import json
import sys
import zipfile

d = json.loads(open(sys.argv[1], encoding='utf-8').read())
if isinstance(d, str):
    d = json.loads(d)
for lcsc, v in d.items():
    if 'b64' not in v:
        print(lcsc, v)
        continue
    z = zipfile.ZipFile(io.BytesIO(base64.b64decode(v['b64'])))
    names = [n for n in z.namelist() if n.endswith('.esym')]
    if not names:
        print(lcsc, v.get('mpn'), 'no .esym in', z.namelist())
        continue
    pins, attrs = {}, {}
    for line in z.read(names[0]).decode('utf-8').splitlines():
        try:
            rec = json.loads(line)
        except ValueError:
            continue
        if rec[0] == 'PIN':
            pins[rec[1]] = rec
        elif rec[0] == 'ATTR' and len(rec) > 4:
            # ["ATTR", id, parentId, key, value, ...]
            attrs.setdefault(rec[2], {})[rec[3]] = rec[4]
    rows = []
    for pid, rec in pins.items():
        a = attrs.get(pid, {})
        rows.append((str(a.get('NUMBER', '?')), str(a.get('NAME', '?'))))

    def key(r):
        n = r[0]
        return (0, int(n)) if n.isdigit() else (1, n)
    rows.sort(key=key)
    print('%s %s (%s): %d pins' % (lcsc, v.get('mpn'), v.get('sym'), len(rows)))
    print('   ' + ', '.join('%s=%s' % r for r in rows))
