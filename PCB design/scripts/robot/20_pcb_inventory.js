// Robot carrier v0.4 PCB — step P5a (from the pcb-pipeline template): inventory every footprint (READ-ONLY): position, rotation, layer, bbox, pads + nets.
// Line per part: DES|footprint|x,y|rot|layer|bbox w x h (centre offset dx,dy)|pad:net@dx,dy;...  (pad offsets
// relative to the component origin, mil). Pattern: WRO hub 20_pcb_inventory.js, plus a project gate.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\20_pcb_inventory.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\data\pcb_inventory_import.json"
const PROJECT = 'WRO_Robot';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT, project: proj && (proj.friendlyName || proj.name) });
const t = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = ((t && t.tabs) || []).map((x) => ({ title: x.title, id: x.tabId, doctype: x.data && x.data.doctype }));
const tab = tabs.find((x) => /pcb/i.test(x.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open', tabs });
await eda.dmt_EditorControl.activateDocument(tab.id);
{ const _t = Date.now(); while (Date.now() - _t < 800) await eda.dmt_Project.getCurrentProjectInfo(); } // no timers (hidden window)
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids);
const r1 = (v) => Math.round(v * 10) / 10;
const lines = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator();
  const x = c.getState_X(), y = c.getState_Y();
  let bb = null;
  try { bb = await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]); } catch (e) { bb = null; }
  const fp = (c.getState_Footprint() || {}).name || '?';
  const pads = (await c.getAllPins()) || [];
  const ps = pads.map((p) => `${p.getState_PadNumber()}:${p.getState_Net() || '-'}@${r1(p.getState_X() - x)},${r1(p.getState_Y() - y)}`);
  const box = bb ? `${r1(bb.maxX - bb.minX)}x${r1(bb.maxY - bb.minY)}(${r1((bb.maxX + bb.minX) / 2 - x)},${r1((bb.maxY + bb.minY) / 2 - y)})` : '?';
  lines.push(`${des}|${fp}|${r1(x)},${r1(y)}|${c.getState_Rotation()}|${c.getState_Layer()}|${box}|${ps.join(';')}`);
}
return JSON.stringify({ tab: tab.title, count: lines.length, lines: lines.sort() });
