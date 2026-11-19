"""Real builder/archived-source tests; never invokes EasyEDA."""
import contextlib
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest.mock import patch

import build_xt_fix as builder

HERE = Path(__file__).resolve().parent
ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_association_check_20261028-1324.epro2'


class BuilderTests(unittest.TestCase):
    def build(self, fp):
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp)
            (target / '54_fix_xt_footprints.js').write_text(
                (HERE / '54_fix_xt_footprints.js').read_text(encoding='utf-8'), encoding='utf-8')
            args = ['build_xt_fix.py', '--archive', str(ARCHIVE), '--footprint', fp]
            with patch.object(builder, 'HERE', target), patch.object(builder, 'ARCHIVE', ARCHIVE), \
                    patch.object(sys, 'argv', args), contextlib.redirect_stdout(io.StringIO()):
                builder.main()
            text = (target / '54_run.js').read_text(encoding='utf-8')
            return json.loads(re.search(r'^const C = (.+);$', text, re.M)[1])

    def test_selected_footprint_is_the_only_write_scope(self):
        c = self.build('eac10ca20e6778ba')
        self.assertEqual([x['uuid'] for x in c['footprints']], ['eac10ca20e6778ba'])

    def test_fresh_saved_centres_and_before_sizes_reach_the_writer(self):
        c = self.build('eac10ca20e6778ba')
        pad = c['footprints'][0]['pads']['e10']
        self.assertEqual(pad.get('xBefore'), -141.7)
        self.assertEqual(pad.get('yBefore'), 0)
        self.assertEqual(pad['padBefore'], 216.5)
        self.assertEqual(pad['holeBefore'], 181.102)
        self.assertEqual(pad['holeAfter'], 157.48)

    def test_xt30_saved_pad_id_and_approved_target_are_retained(self):
        c = self.build('9197bb2e6c38da76')
        self.assertEqual([x['uuid'] for x in c['footprints']], ['9197bb2e6c38da76'])
        self.assertEqual(set(c['footprints'][0]['pads']), {'e18', 'e19'})
        self.assertEqual(c['footprints'][0]['pads']['e18']['holeAfter'], 98.425)
        self.assertEqual(c['footprints'][0]['pads']['e18']['padAfter'], 137.795)


if __name__ == '__main__':
    unittest.main()
