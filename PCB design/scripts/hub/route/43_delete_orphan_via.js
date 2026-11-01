// Hub v0.4 PCB — remove ONE orphan GND stitching via (WRITES: deletes that via only). gnd_stage.py put it at
// (2715, 2780) between U1B's pin row and the station block, where the GND fills ended up absent on both layers, so
// EasyEDA's DRC reported it "disconnected" (2026-10-27). Deletes only a GND via at exactly that spot with the planned
// size (0.4 / 0.8 mm); anything else there aborts. Idempotent (nothing to delete -> ok).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\route\43_delete_orphan_via.js"
const PROJECT = 'WRO_HUB';
const X = 2715, Y = 2780;
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const hit = ((await eda.pcb_PrimitiveVia.getAll()) || []).filter((v) => Math.abs(v.getState_X() - X) < 0.6 && Math.abs(v.getState_Y() - Y) < 0.6);
if (!hit.length) return JSON.stringify({ ok: true, deleted: 0 });
if (hit.length > 1 || hit[0].getState_Net() !== 'GND' || Math.abs(hit[0].getState_Diameter() - 31.5) > 1)
  return JSON.stringify({ abort: 'unexpected via(s) at the spot', found: hit.map((v) => [v.getState_Net(), v.getState_HoleDiameter(), v.getState_Diameter()]) });
await eda.pcb_PrimitiveVia.delete([hit[0].getState_PrimitiveId()]);
await eda.pcb_Document.save(tab.tabId);
const left = ((await eda.pcb_PrimitiveVia.getAll()) || []).filter((v) => Math.abs(v.getState_X() - X) < 0.6 && Math.abs(v.getState_Y() - Y) < 0.6).length;
return JSON.stringify({ ok: left === 0, deleted: 1 });
