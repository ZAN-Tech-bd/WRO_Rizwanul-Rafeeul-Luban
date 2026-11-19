"""Regression checks: never use historical layout geometry for a manually edited PCB."""
import copy
import unittest

from manual_model import build_model


def fixture():
    pad = dict(id='pad1', num='1', net='GND', x=110, y=220,
               rotation=90, layer=12, shape=['RECT', 60, 40], hole=['ROUND', 30], plated=True)
    return dict(project='station_pcb', components=[dict(des='J1', pads=[dict(pad)])],
                pads=[pad, dict(id='hole1', num='H1', net='', x=157, y=157, rotation=0,
                               layer=12, shape=['ELLIPSE', 236, 236], hole=['ROUND', 126], plated=True)],
                polylines=[dict(layer=11, source=[0, 0, 'L', 0, 1255, 2000, 1255, 2000, 0, 0, 0])],
                fills=[], pours=[], regions=[])


class LiveModelTests(unittest.TestCase):
    def test_current_outline_and_holes_not_old_layout(self):
        pads, holes, board = build_model(fixture())
        self.assertEqual(board.bounds, (0, 0, 2000, 1255))
        self.assertEqual((holes[0]['x'], holes[0]['y'], holes[0]['d'], holes[0]['ring']), (157, 157, 126, 236))
        self.assertEqual(len(pads), 1)

    def test_absolute_pad_rotation_and_drill(self):
        pads, _, _ = build_model(fixture())
        p = pads[0]
        self.assertEqual((p.cx, p.cy, p.w, p.h, p.hole), (110, 220, 40, 60, 30))
        self.assertEqual(p.layers, (0, 1))
        self.assertEqual((p.des, p.num), ('J1', '1'))

    def test_uses_standalone_drill_not_component_proxy(self):
        d = fixture()
        d['components'][0]['pads'][0]['hole'] = ['ROUND', 3, 3]
        self.assertEqual(build_model(d)[0][0].hole, 30)

    def test_reject_unknown_copper_geometry(self):
        d = fixture()
        d['polylines'].append(dict(layer=2, source=[0, 0, 'L', 10, 10]))
        with self.assertRaises(ValueError):
            build_model(d)

    def test_reject_nonrectangular_outline(self):
        d = fixture()
        d['polylines'][0]['source'] = [0, 0, 'L', 0, 1255, 2000, 1255, 1800, 0, 0, 0]
        with self.assertRaises(ValueError):
            build_model(d)

    def test_reject_unhandled_keepout(self):
        d = fixture()
        d['regions'] = [dict(layer=2)]
        with self.assertRaises(ValueError):
            build_model(d)

    def test_reject_missing_absolute_component_pad(self):
        d = fixture()
        d['pads'] = d['pads'][1:]
        with self.assertRaises(ValueError):
            build_model(d)


if __name__ == '__main__':
    unittest.main()
