// Station board v0.4 PCB (from the pcb-pipeline template) — dump of the GND Fill regions actually in PCB1 (READ-ONLY).
// Returns [[layer 1|2, complexPolygonSource], ...]: the read-back input of check_fills.py (D-038: EasyEDA's DRC does not
// see fill overlaps, so every fill write and every copper edit is followed by this dump + check_fills.py).
// Run (redirect into data/): node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\29b_dump_fills.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\data\_eda_fills_now.json"
const PROJECT = 'station_pcb';
const PROJECT_OK = (p) => p === PROJECT;
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || !PROJECT_OK(proj.friendlyName || proj.name)) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const fills = ((await eda.pcb_PrimitiveFill.getAll()) || []).filter((f) => f.getState_Net() === 'GND');
return JSON.stringify(fills.map((f) => [Number(f.getState_Layer()), f.getState_ComplexPolygon().getSource()]));
