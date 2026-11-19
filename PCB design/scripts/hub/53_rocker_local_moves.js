// Hub v0.4 — H-001 retry 2, step 4 (WRITES PCB1 of WRO_HUB): make room for the direct-mount KCD4-203-B (J3) with LOCAL
// moves only (user 2026-10-26: "adjust the placement a bit, not replacing all things, just the overlapping things").
// Planned and checked offline by live_space.py (option A, 0 problems, preview data/live_space_rocker_option_a.png):
//   left board edge x -300 -> -620 mil (124.5 -> 132.6 mm wide; height 99.1 mm unchanged);
//   J3 (-10, 1446) -> (-60, 1495), rotation -90 kept;  J4 (-10, 640) -> (-10, 445);  J5 y 638.3 -> 513.3;
//   LED3 (330, 190) -> (780, 190);  left M3 holes (-130, 160) -> (-450, 160) and (170, 3730) -> (-450, 3730).
// Every other part, the right holes and all silk stay where the user put them. Each target is checked at its "from"
// position first: if anything moved since the plan (user edit), the script aborts before writing. Idempotent: items
// already at their target are skipped. Saves PCB1 at the end; verify with export_project.py + inspect_export.py.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\53_rocker_local_moves.js"
const PROJECT = 'WRO_HUB', PCB = 'ad35aed4d7610fd2';
const PARTS = { J3: [[-10, 1446], [-60, 1495], -90], J4: [[-10, 640], [-10, 445], 90],
                J5: [[343.913, 638.323], [343.913, 513.323], 90], LED3: [[330, 190], [780, 190], 0] };
const HOLES = { '546cc19edcea79a7': [[-130, 160], [-450, 160]], c6bc3262935c9d26: [[170, 3730], [-450, 3730]] };
const OLD_OUT = [-300, 0, 4600, 3900], NEW_OUT = [-620, 0, 4600, 3900];
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || proj.friendlyName !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => t.tabId.startsWith(PCB + '@'));
if (!tab) return JSON.stringify({ abort: 'PCB1 tab not open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (!doc || doc.uuid !== PCB || doc.documentType !== 3) return JSON.stringify({ abort: 'PCB1 not active', doc });
const at = (x, y, p, t = 0.5) => Math.abs(x - p[0]) < t && Math.abs(y - p[1]) < t;
const res = { moved: [], ok: [], holesMoved: [], holesOk: [], outline: '', errors: [] };
// ---- pre-check everything, write nothing on a mismatch
const comps = ((await eda.pcb_PrimitiveComponent.getAll()) || []).filter((c) => PARTS[c.getState_Designator()]);
if (comps.length !== 4) return JSON.stringify({ abort: 'found ' + comps.map((c) => c.getState_Designator()) });
const todo = [];
for (const c of comps) {
  const d = c.getState_Designator(), [from, to, rot] = PARTS[d], x = c.getState_X(), y = c.getState_Y();
  if (Math.round(((c.getState_Rotation() - rot) % 360 + 360) % 360) % 360 !== 0) return JSON.stringify({ abort: d + ' rotation ' + c.getState_Rotation() });
  if (at(x, y, to)) { res.ok.push(d); continue; }
  if (!at(x, y, from)) return JSON.stringify({ abort: d + ' is at ' + x + ',' + y + ', expected ' + from + ' (board changed: re-plan)' });
  todo.push([c, d, to]);
}
const j3 = comps.find((c) => c.getState_Designator() === 'J3');
const nets = {};
for (const p of await j3.getAllPins()) nets[p.getState_PadNumber()] = p.getState_Net();
const J3_NETS = { 1: 'BAT+', 2: 'V12', 3: 'DC+', 4: 'BAT-', 5: 'GND', 6: 'DC-' };
if (Object.keys(nets).length !== 6 || Object.entries(J3_NETS).some(([n, net]) => nets[n] !== net))
  return JSON.stringify({ abort: 'J3 nets ' + JSON.stringify(nets) });
const pads = (await eda.pcb_PrimitivePad.getAll()) || [];
const holeTodo = [];
for (const [id, [from, to]] of Object.entries(HOLES)) {
  const p = pads.find((q) => q.getState_PrimitiveId() === id);
  if (!p) return JSON.stringify({ abort: 'hole ' + id + ' missing' });
  if (at(p.getState_X(), p.getState_Y(), to)) { res.holesOk.push(id); continue; }
  if (!at(p.getState_X(), p.getState_Y(), from)) return JSON.stringify({ abort: 'hole ' + id + ' at ' + p.getState_X() + ',' + p.getState_Y() });
  holeTodo.push([id, to]);
}
const extOf = (q) => { const v = q.getState_Polygon().getSource().flat(Infinity).filter((n) => typeof n === 'number');
  const xs = v.filter((_, i) => i % 2 === 0), ys = v.filter((_, i) => i % 2 === 1);
  return [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)]; };
const outl = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((q) => String(q.getState_Layer()) === '11');
if (outl.length !== 1) return JSON.stringify({ abort: outl.length + ' outline polylines, expected 1' });
const ext = extOf(outl[0]), same = (a, b) => a.every((v, i) => Math.abs(v - b[i]) < 0.5);
if (!same(ext, NEW_OUT) && !same(ext, OLD_OUT)) return JSON.stringify({ abort: 'outline extents ' + ext });
// ---- writes
for (const [c, d, to] of todo) {
  try { await eda.pcb_PrimitiveComponent.modify(c.getState_PrimitiveId(), { x: to[0], y: to[1] }); res.moved.push(d); }
  catch (e) { res.errors.push(d + ' ' + String(e).slice(0, 100)); }
}
for (const [id, to] of holeTodo) {
  try { await eda.pcb_PrimitivePad.modify(id, { x: to[0], y: to[1] }); res.holesMoved.push(id); }
  catch (e) { res.errors.push('hole ' + id + ' ' + String(e).slice(0, 100)); }
}
if (same(ext, NEW_OUT)) res.outline = 'already ' + NEW_OUT;
else {
  const [x0, y0, x1, y1] = NEW_OUT;
  const o = await eda.pcb_PrimitivePolyline.create('', 11, eda.pcb_MathPolygon.createPolygon([x0, y0, 'L', x1, y0, x1, y1, x0, y1, x0, y0]), 10);
  if (o) { await eda.pcb_PrimitivePolyline.delete([outl[0].getState_PrimitiveId()]); res.outline = 'replaced ' + ext + ' -> ' + NEW_OUT; }
  else res.errors.push('outline create returned nothing (old outline kept)');
}
await eda.pcb_Document.save();
// ---- readback
res.readBack = {};
for (const c of ((await eda.pcb_PrimitiveComponent.getAll()) || []).filter((q) => PARTS[q.getState_Designator()]))
  res.readBack[c.getState_Designator()] = [c.getState_X(), c.getState_Y(), c.getState_Rotation()];
for (const p of ((await eda.pcb_PrimitivePad.getAll()) || []).filter((q) => HOLES[q.getState_PrimitiveId()]))
  res.readBack[p.getState_PrimitiveId()] = [p.getState_X(), p.getState_Y()];
res.readBack.outline = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((q) => String(q.getState_Layer()) === '11').map(extOf);
return JSON.stringify(res);
