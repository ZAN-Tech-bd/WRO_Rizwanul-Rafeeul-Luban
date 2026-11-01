"""RF correction gates: keep width/endpoints, clear actual antenna and foreign copper."""
import unittest
from pathlib import Path
import repaired_hub as R


SOURCE=Path(__file__).resolve().parents[2]/'hub/WRO_HUB_REPAIRED_silk_positioned_20261027-2019.epro2'


class RfTests(unittest.TestCase):
    def test_local_reroute_preserves_width_and_endpoints_and_clears_antenna(self):
        fn=getattr(R,'rf_plan',None)
        self.assertTrue(callable(fn),'Missing guarded RF repair planner')
        from shapely.geometry import LineString,Point,box
        from shapely.ops import unary_union
        m=R.copper_model(SOURCE);plan=fn(m)
        self.assertEqual(set(plan['modify']),{'3b4f9ab3221e7d4d','84b60016a78b71de'})
        self.assertEqual(len(plan['add']),1)
        lines=list(plan['modify'].values())+plan['add']
        for line in lines:
            self.assertEqual((line['net'],line['layer'],line['width']),('+3V3',2,31.4961))
        shape=unary_union([LineString([(q['x1'],q['y1']),(q['x2'],q['y2'])]).buffer(q['width']/2,quad_segs=64) for q in lines])
        self.assertTrue(shape.covers(Point(2250,3810)))
        self.assertTrue(shape.covers(Point(2680,3915)))
        # Pin1 y=3810 + drawing's 1.91mm upper-body offset; not the much larger plane keep-out.
        antenna=box(1475,3885.196850393701,2595,4075)
        self.assertGreater(shape.distance(antenna),4)
        gaps=[shape.distance(i['shape']) for i in m['items'] if i['net']!='+3V3' and 2 in i['layers']]
        self.assertGreaterEqual(min(gaps),6)


if __name__=='__main__':unittest.main()
