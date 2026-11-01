// Hub v0.4 PCB — routing prep (READ-ONLY): the LIVE board for the offline router, straight from EasyEDA (the user's
// hand placement no longer matches layout_plan.py, and J3 / the XT pads are not library footprints).
// Returns {comps: [{des, x, y, rot, pads: [{num, net, x, y, rot, layer, pad, hole, holeRot, holeOx, holeOy, plated}]}],
//          free: [{id, num, net, x, y, rot, layer, pad, hole}], outline: [[x, y], ...] (layer-11 polylines)}.
// Units as EasyEDA returns them: positions / pad sizes in mil; component-pad holes in 10-mil units, free-pad holes in mil
// (playbook §7) — the Python side converts.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\20e_live_pads.js" > data\route_20261027\live.json
const PROJECT = 'WRO_HUB_REPAIR_ISOLATED_20261027';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const safe = (f) => { try { return f(); } catch (e) { return null; } };
const padRec = (p) => ({
  num: String(p.getState_PadNumber()), net: p.getState_Net() || '', x: p.getState_X(), y: p.getState_Y(),
  rot: safe(() => p.getState_Rotation()), layer: safe(() => p.getState_Layer()), pad: safe(() => p.getState_Pad()),
  hole: safe(() => p.getState_Hole()), holeRot: safe(() => p.getState_HoleRotation()),
  holeOx: safe(() => p.getState_HoleOffsetX()), holeOy: safe(() => p.getState_HoleOffsetY()),
  plated: safe(() => p.getState_Metallization()), id: safe(() => p.getState_PrimitiveId()) });
const comps = [];
const compPadIds = new Set();
for (const c of (await eda.pcb_PrimitiveComponent.getAll()) || []) {
  const pins = (await c.getAllPins()) || [];
  const pads = pins.map(padRec);
  pads.forEach((p) => compPadIds.add(p.id));
  comps.push({ des: c.getState_Designator(), x: c.getState_X(), y: c.getState_Y(), rot: c.getState_Rotation(),
               layer: c.getState_Layer(), pads });
}
const free = ((await eda.pcb_PrimitivePad.getAll()) || []).map(padRec).filter((p) => !compPadIds.has(p.id));
const outline = [];
for (const q of (await eda.pcb_PrimitivePolyline.getAll()) || []) {
  if (Number(q.getState_Layer()) !== 11) continue;
  outline.push(q.getState_Polygon().getSource());
}
return JSON.stringify({ comps, free, outline });
