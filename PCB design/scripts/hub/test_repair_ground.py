"""Fail-closed ground regeneration against current saved pad/route geometry."""
import unittest
import tempfile
from pathlib import Path
from unittest.mock import patch
import repaired_hub as R


SOURCE=Path(__file__).resolve().parents[2]/'hub/WRO_HUB_REPAIRED_rf_fixed_20261027-2022.epro2'


class GroundTests(unittest.TestCase):
    def test_regenerated_plane_clears_current_copper_and_connects_all_ground(self):
        fn=getattr(R,'regenerate_ground',None)
        self.assertTrue(callable(fn),'Missing isolated-project ground regeneration')
        # Historical fixtures must never overwrite the active user's ground plan or evidence.
        with tempfile.TemporaryDirectory() as folder,patch.object(R,'DATA',Path(folder)):
            plan,report=fn(SOURCE)
        self.assertEqual(report['foreignOverlaps'],[])
        self.assertEqual(report['clearanceViolations'],[])
        self.assertEqual(report['disconnectedGnd'],[])
        self.assertGreaterEqual(report['minimumClearanceMil']['normal'],11.8)
        self.assertGreaterEqual(report['minimumClearanceMil']['left'],39.8)
        self.assertGreaterEqual(report['minimumClearanceMil']['source'],49.8)
        self.assertGreaterEqual(report['edgeClearanceMil'],15.8)
        self.assertGreaterEqual(report['antennaClearanceMil'],7.8)
        self.assertGreater(len(plan['pieces']),1000)


if __name__=='__main__':unittest.main()
