// Attendance carrier v0.3 PCB — dump of the copper actually in PCB1 (READ-ONLY).
// Returns {net: {segs: [[layer 1|2, x1, y1, x2, y2, width, id]], vias: [[x, y, hole, dia, id]],
// arcs: [[layer 1|2, x1, y1, x2, y2, arcAngle, width, id]]}} for every top/bottom track, via and arc
// (net '' = copper without a net) — the input of check_eda_copper.py, pour_fill.py and check_fills.py.
// Arcs exist since the user's rev D routing edits (rounded power corners, 2026-10-17); check_eda_copper.load()
// flattens them into chord polylines, so older consumers that ignore the extra key keep working.
// Run (redirect into data/): node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\29_dump_copper.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\data\_eda_now.json"
const PROJECT = 'WRO_HUB_REPAIR_ISOLATED_20261027';
const PROJECT_OK = (p) => ['WRO_HUB_REPAIR_ISOLATED_20261027'].includes(p);
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || !PROJECT_OK(proj.friendlyName || proj.name)) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);   // pcb_* reads fail while a schematic tab is active
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const L = ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => ['1', '2'].includes(String(l.getState_Layer())));
const V = (await eda.pcb_PrimitiveVia.getAll()) || [];
const A = ((await eda.pcb_PrimitiveArc.getAll()) || []).filter((a) => ['1', '2'].includes(String(a.getState_Layer())));
const by = {};
const g = (n) => (by[n] = by[n] || { segs: [], vias: [], arcs: [] });
for (const l of L) {
  g(l.getState_Net() || '').segs.push([Number(l.getState_Layer()), l.getState_StartX(), l.getState_StartY(), l.getState_EndX(),
    l.getState_EndY(), l.getState_LineWidth(), l.getState_PrimitiveId()]);
}
for (const v of V) g(v.getState_Net() || '').vias.push([v.getState_X(), v.getState_Y(), v.getState_HoleDiameter(), v.getState_Diameter(), v.getState_PrimitiveId()]);
for (const a of A) {
  g(a.getState_Net() || '').arcs.push([Number(a.getState_Layer()), a.getState_StartX(), a.getState_StartY(), a.getState_EndX(),
    a.getState_EndY(), a.getState_ArcAngle(), a.getState_LineWidth(), a.getState_PrimitiveId()]);
}
return JSON.stringify(by);
