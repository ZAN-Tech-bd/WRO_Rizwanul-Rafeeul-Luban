// Hub v0.4 — H-001 retry 2: J3 KCD4-203-B footprint in the in-place footprint editor (WRITES the open footprint-editor
// document only; does NOT save, does NOT touch PCB1). The user opens it: PCB1 -> right-click J3 -> Edit Footprint.
// Geometry = WIRING-PLAN §2.4 / rocker_model.py (tested): pins 1-3 at x = -10.5 / 0 / +10.5 mm, y = +5.5; pins 4-6 at y = -5.5.
// Pads OVAL 3.0 (x) x 9.0 (y) mm; plated SLOT 1.8 mm wide x 7.8 mm long, long axis along y (pole spacing).
// Uses the documented primitive APIs (pcb_PrimitivePad.modify etc.), NOT setDocumentSource: a source replacement
// showed on the canvas but saved as a footprint with zero pads (H-001). Keeps pad ids e21..e26 so PAD_NET rows keep
// their nets. Old silk / body / pin fills are deleted; flange silk, body shape, courtyard and a pin-1 "1" are drawn.
// Free pads in this editor report the hole in mil (70.866 = 1.8 mm), unlike component pads (10-mil units).
// Idempotent: pads already at target are skipped, drawn items are recognised by geometry. Readback = native source.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\52_rocker_fp_editor.js"
const PROJECT = 'WRO_HUB', TARGET = 'ad35aed4d7610fd2_0cbe9290d583d889';
const MM = 1 / 0.0254;
const PADS = { e26: ['1', -10.5, 5.5], e25: ['2', 0, 5.5], e24: ['3', 10.5, 5.5],
               e23: ['4', -10.5, -5.5], e22: ['5', 0, -5.5], e21: ['6', 10.5, -5.5] };
const PAD = ['OVAL', 3.0 * MM, 9.0 * MM], HOLE = ['SLOT', 1.8 * MM, 7.8 * MM], HOLE_ROT = 90;
const OLD_POLYS = ['e12', 'e13', 'e14', 'e15', 'e16', 'e17', 'e18', 'e19', 'e20'];
const OLD_FILLS = ['e3', 'e4', 'e5', 'e6', 'e7', 'e8', 'e9', 'e10', 'e11'];
const RECTS = [[3, 34.5, 25.5, 0.2], [48, 34.5, 25.5, 0.1], [13, 36.5, 27.5, 0.1]];   // silk flange, body, courtyard
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (!proj || proj.friendlyName !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (!doc || doc.documentType !== 4 || doc.uuid !== TARGET) return JSON.stringify({ abort: 'active tab is not the J3 footprint editor', doc });
const near = (a, b, t = 0.05) => Math.abs(a - b) < t;
const pads = await eda.pcb_PrimitivePad.getAll();
if (pads.length !== 6) return JSON.stringify({ abort: pads.length + ' pads, expected 6' });
for (const p of pads) {
  const want = PADS[p.getState_PrimitiveId()];
  if (!want || String(p.getState_PadNumber()) !== want[0]) return JSON.stringify({ abort: 'pad id/number mismatch ' + p.getState_PrimitiveId() + '/' + p.getState_PadNumber() });
}
const res = { padsChanged: [], padsOk: [], deleted: [], drawn: [], errors: [] };
for (const p of pads) {
  const id = p.getState_PrimitiveId(), [, x, y] = PADS[id];
  const s = p.getState_Pad(), h = p.getState_Hole();
  if (near(p.getState_X(), x * MM) && near(p.getState_Y(), y * MM) && s[0] === 'OVAL' && near(s[1], PAD[1]) && near(s[2], PAD[2])
      && h && h[0] === 'SLOT' && near(h[1], HOLE[1]) && near(h[2], HOLE[2]) && p.getState_HoleRotation() === HOLE_ROT) { res.padsOk.push(id); continue; }
  try {
    await eda.pcb_PrimitivePad.modify(id, { x: x * MM, y: y * MM, rotation: 0, pad: PAD, hole: HOLE,
      holeOffsetX: 0, holeOffsetY: 0, holeRotation: HOLE_ROT, metallization: true });
    res.padsChanged.push(id);
  } catch (e) { res.errors.push(id + ' ' + String(e).slice(0, 100)); }
}
const polys = await eda.pcb_PrimitivePolyline.getAll();
const oldPolys = polys.map((q) => q.getState_PrimitiveId()).filter((i) => OLD_POLYS.includes(i));
if (oldPolys.length) { await eda.pcb_PrimitivePolyline.delete(oldPolys); res.deleted.push(...oldPolys); }
const oldFills = (await eda.pcb_PrimitiveFill.getAll()).map((q) => q.getState_PrimitiveId()).filter((i) => OLD_FILLS.includes(i));
if (oldFills.length) { await eda.pcb_PrimitiveFill.delete(oldFills); res.deleted.push(...oldFills); }
const rectPath = (w, h) => { const x = w * MM / 2, y = h * MM / 2; return [-x, -y, 'L', x, -y, x, y, -x, y, -x, -y]; };
const bboxOf = (q) => { const p = q.getState_Polygon().getSource().flat(Infinity).filter((v) => typeof v === 'number');
  const xs = p.filter((_, i) => i % 2 === 0), ys = p.filter((_, i) => i % 2 === 1);
  return [Math.max(...xs) - Math.min(...xs), Math.max(...ys) - Math.min(...ys)]; };
const have = (await eda.pcb_PrimitivePolyline.getAll()).filter((q) => !OLD_POLYS.includes(q.getState_PrimitiveId()));
for (const [layer, w, h, lw] of RECTS) {
  if (have.some((q) => q.getState_Layer() === layer && (([bw, bh]) => near(bw, w * MM, 1) && near(bh, h * MM, 1))(bboxOf(q)))) continue;
  const made = await eda.pcb_PrimitivePolyline.create('', layer, eda.pcb_MathPolygon.createPolygon(rectPath(w, h)), lw * MM);
  if (made) res.drawn.push('rect L' + layer + ' ' + w + 'x' + h); else res.errors.push('rect L' + layer + ' refused');
}
if (!(await eda.pcb_PrimitiveString.getAll(3)).some((q) => q.getState_Text() === '1')) {
  const made = await eda.pcb_PrimitiveString.create(3, -14.25 * MM, 5.5 * MM, '1', 'default', 1.5 * MM, 0.2 * MM, 5, 0, false, 0, false, false);
  if (made) res.drawn.push('silk "1" at pin 1'); else res.errors.push('pin-1 text refused');
}
// Readback from the native document source (what save serialises), not from getters alone.
const src = await eda.sys_FileManager.getDocumentSource();
const recs = src.split('\n').map((l) => { const i = l.indexOf('||'); if (i < 0) return null;
  try { return [JSON.parse(l.slice(0, i)), JSON.parse(l.slice(i + 2).replace(/\|\s*$/, ''))]; } catch (e) { return null; } }).filter(Boolean);
res.readBack = recs.filter(([a]) => a.type === 'PAD').map(([a, b]) => ({ id: a.id, num: b.num,
  xy_mm: [+(b.centerX * 0.0254).toFixed(3), +(b.centerY * 0.0254).toFixed(3)], padAngle: b.padAngle,
  pad: [b.defaultPad.padType, +(b.defaultPad.width * 0.0254).toFixed(3), +(b.defaultPad.height * 0.0254).toFixed(3)],
  hole: [b.hole && b.hole.holeType, +(b.hole.width * 0.0254).toFixed(3), +(b.hole.height * 0.0254).toFixed(3)],
  relativeAngle: b.relativeAngle, plated: b.plated }));
res.otherRecords = recs.filter(([a]) => ['POLY', 'FILL', 'STRING'].includes(a.type)).map(([a, b]) => a.type + ' ' + a.id + ' L' + b.layerId);
return JSON.stringify(res);
