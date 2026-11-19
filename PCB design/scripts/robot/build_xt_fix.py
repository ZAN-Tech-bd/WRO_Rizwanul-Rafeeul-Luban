"""Build 54_run.js: XT footprint pad/hole corrections from the saved archive (D-031 sizes)."""
import argparse
import json
from pathlib import Path

import gnd_edit

HERE = Path(__file__).resolve().parent
ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_association_check_20261028-1324.epro2'
PLAN = {  # hole (mil), pad (mil or None to keep)
    'eac10ca20e6778ba': dict(hole=157.48, pad=None),       # J1 XT60: 4.0 mm drill, 5.5 pad kept
    '9e78c3c3ca74ad42': dict(hole=98.425, pad=137.795),    # J2/J3 XT30-F: 2.5 / 3.5
    '9197bb2e6c38da76': dict(hole=98.425, pad=137.795),    # J4 XT30-M: 2.5 / 3.5
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=ARCHIVE)
    parser.add_argument('--footprint', choices=tuple(PLAN), required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--save-verified-only', action='store_true',
                        help='Validate already-edited target dimensions and save; issue no pad modifications')
    args = parser.parse_args()
    output = args.output or HERE / '54_run.js'
    assert not output.exists(), 'Refusing to overwrite an existing writer; choose a new --output'
    docs = gnd_edit.read_archive(args.archive)
    footprints = []
    for fp, spec in ((args.footprint, PLAN[args.footprint]),):
        pads = {}
        for (t, i), r in docs[fp]['records'].items():
            if t != 'PAD':
                continue
            b = r['body']
            assert b['hole']['holeType'] == 'ROUND'
            assert b['defaultPad']['padType'] == 'ELLIPSE'
            assert b['defaultPad']['width'] == b['defaultPad']['height']
            pads[i] = dict(num=b['num'], holeBefore=b['hole']['width'],
                           padBefore=b['defaultPad']['width'],
                           xBefore=b['centerX'], yBefore=b['centerY'],
                           holeAfter=spec['hole'], padAfter=spec['pad'])
        assert len(pads) == 2, (fp, pads)
        footprints.append(dict(uuid=fp, pads=pads))
    config = dict(project='7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864',
                  pcb='643ce99542452c8f', footprints=footprints,
                  saveVerifiedOnly=args.save_verified_only)
    src = (HERE / '54_fix_xt_footprints.js').read_text(encoding='utf-8').replace(
        '__CONFIG__', json.dumps(config, separators=(',', ':')))
    with output.open('x', encoding='utf-8') as f:
        f.write(src)
    print('built', output, 'with', sum(len(f['pads']) for f in footprints), 'pads in', len(footprints), 'footprint')


if __name__ == '__main__':
    main()
