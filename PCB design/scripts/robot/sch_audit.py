# Robot carrier v0.4 (copied from attendance) — schematic net-list audit (offline, read-only).
# Compares the net list EasyEDA reports (15_check.js "netList", saved as JSON) with the net map 14_wire.js wires from
# (its NET block, evaluated with node), pin by pin. Both directions: pins missing from a net and pins that are extra.
# Usage: python sch_audit.py data/sch_check_v04.json   (the 15_check.js result, raw JSON string or object)
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def wire_map():
    src = open(os.path.join(HERE, '14_wire.js'), encoding='utf-8').read()
    body = src[src.index('const NET = {};'):src.index('// ---- draw')]
    out = subprocess.run(['node', '-e', body + '\nconsole.log(JSON.stringify(NET));'], capture_output=True, text=True,
                         check=True).stdout
    nets = {}
    for des, pins in json.loads(out).items():
        for num, net in pins.items():
            nets.setdefault(net, set()).add('%s.%s' % (des, num))
    return nets


def sheet_map(path):
    d = json.load(open(path, encoding='utf-8'))
    if isinstance(d, str):
        d = json.loads(d)
    nets = {}
    for line in d['netList']:
        net, pins = line.split(' = ', 1)
        nets[net] = set(pins.split())
    return nets, d


if __name__ == '__main__':
    want = wire_map()
    have, raw = sheet_map(sys.argv[1])
    diffs = []
    for net in sorted(set(want) | set(have)):
        miss, extra = want.get(net, set()) - have.get(net, set()), have.get(net, set()) - want.get(net, set())
        if miss or extra:
            diffs.append((net, sorted(miss), sorted(extra)))
    print('nets: map %d, sheet %d; pins: map %d, sheet %d' % (len(want), len(have), sum(map(len, want.values())),
                                                             sum(map(len, have.values()))))
    print('checks: shorts %s, bare %s, stacked %s, single-pin %s, erc %s' % (
        raw.get('shorts'), raw.get('pinsBare'), raw.get('stackedPins'), raw.get('singlePinNets'), raw.get('erc')))
    print('differences: %d' % len(diffs))
    for net, miss, extra in diffs:
        print('  %-12s missing %s  extra %s' % (net, miss, extra))
    sys.exit(1 if diffs else 0)
