"""GND regeneration gates for the edited Robot board (hub final_ground discipline).

robot_release.ground_plan()/serialize()/verify_plan() must: derive fills from CURRENT
geometry, exclude BOTH the historical conservative strip and the ACTUAL on-board antenna
overhang, connect every GND pad/via/track through the fills with zero floating pieces,
clear all foreign items by >= NORMAL-0.5 mil, stay >= EDGE-0.2 from the outline edge,
and serialize to valid hole-free polygons.
"""
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / 'hub'))

ARCHIVE = HERE.parents[1] / 'Robot/WRO_Robot_via_deleted_20261028-1438.epro2'
_MODEL = {}


def model():
    if 'm' not in _MODEL:
        import robot_release as R
        _MODEL['m'] = R.copper_model(ARCHIVE)
    return _MODEL['m']


def plan_pieces():
    import robot_release as R
    m = model()
    fills = R.ground_plan(m)
    return m, fills, R.serialize(fills)


class GroundPlanGates(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.m, cls.fills, cls.pieces = plan_pieces()

    def test_every_fill_piece_is_a_valid_hole_free_polygon(self):
        import final_ground as FG
        for p in self.pieces:
            q = FG.from_source(p['source'])
            self.assertTrue(q.is_valid, p['source'][:12])
            self.assertEqual(len(q.interiors), 0)
            self.assertGreaterEqual(q.area, 4)

    def test_fills_stay_inside_the_outline_with_edge_margin(self):
        import robot_release as R
        for l, polys in self.fills.items():
            for g in polys:
                self.assertTrue(self.m['outline'].covers(g), (l, g.bounds))
        edge = min(g.distance(self.m['outline'].exterior) for gs in self.fills.values() for g in gs)
        self.assertGreaterEqual(edge, R.EDGE - 0.2)

    def test_actual_antenna_overhang_has_no_fill(self):
        import rf_repair
        ant = rf_repair.antenna_box(self.m)
        for g in (q for gs in self.fills.values() for q in gs):
            self.assertFalse(g.intersects(ant), g.bounds)

    def test_no_floating_pieces_and_every_gnd_item_connected(self):
        import final_ground as FG
        connected, disconnected = FG.connectivity(self.fills, self.m)
        floating = sum(len(self.fills[l]) - len(connected[l]) for l in (1, 2))
        self.assertEqual(floating, 0)
        self.assertEqual(disconnected, [])

    def test_verify_plan_report_is_clean_and_enforces_clearances(self):
        import robot_release as R
        report = R.verify_plan(self.m, self.pieces)
        self.assertEqual(report['foreignOverlaps'], [])
        self.assertEqual(report['clearanceViolations'], [])
        self.assertEqual(report['disconnectedGnd'], [])
        self.assertEqual(report['floatingFillPieces'], 0)
        self.assertGreaterEqual(report['minForeignGapMil'], R.NORMAL - 0.5)


if __name__ == '__main__':
    unittest.main()
