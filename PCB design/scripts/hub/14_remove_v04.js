// Hub v0.4 — step 3h: remove placed schematic parts for good (WRITES the schematic).
// v0.4 (2026-10-25, user-approved part-count review): R14 (E-stop 10 k pull-down) and R21–R27 (7 x 10 k DHT pull-ups).
// Deletes each part's pin stubs (09_wire_nets.js gives every pin its own named stub), then the part; 07_place.js /
// 09_wire_nets.js no longer list them. Idempotent (gone = reported). From robot 17b_remove_v05.js / the pcb-pipeline
// template. Gates: WRO_HUB, schematic tab, window visible.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\14_remove_v04.js"
const REMOVE = ['R14', 'R21', 'R22', 'R23', 'R24', 'R25', 'R26', 'R27'];
const PROJECT = 'WRO_HUB';
// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(800);
const t0 = Date.now();
const res = [];
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const all = (await eda.sch_PrimitiveComponent.get(ids)).filter((c) => c && c.getState_Designator);
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const ends = (w) => { const f = (w.getState_Line() || []).flat(3); return [[f[0], f[1]], [f[f.length - 2], f[f.length - 1]]]; };
for (const des of REMOVE) {
  if (Date.now() - t0 > 18000) { res.push({ des, skipped: 'time box' }); continue; }
  const comps = all.filter((c) => c.getState_Designator() === des);
  if (!comps.length) { res.push({ des, gone: true }); continue; }
  if (comps.length > 1) { res.push({ des, abort: 'found ' + comps.length + ' parts with this designator; nothing changed' }); continue; }
  const c = comps[0];
  const pins = (await c.getAllPins()) || [];
  const onPin = ([ex, ey]) => pins.some((p) => Math.abs(ex - p.getState_X()) < 0.5 && Math.abs(ey - p.getState_Y()) < 0.5);
  const stubs = wires.filter((w) => ends(w).some(onPin));
  const names = stubs.map((w) => (w.getState_Net && w.getState_Net()) || '?');
  if (stubs.length) await eda.sch_PrimitiveWire.delete(stubs.map((w) => w.getState_PrimitiveId()));
  await eda.sch_PrimitiveComponent.delete([c.getState_PrimitiveId()]);
  await sleep(500);
  res.push({ des, removed: true, at: [Math.round(c.getState_X()), Math.round(c.getState_Y())], pins: pins.length,
    stubsRemoved: stubs.length, stubNets: names });
}
await sleep(1000);
await eda.sch_Document.save();
const left = (await eda.sch_PrimitiveComponent.get(await eda.sch_PrimitiveComponent.getAllPrimitiveId()))
  .filter((c) => c && c.getState_Designator && REMOVE.includes(c.getState_Designator())).map((c) => c.getState_Designator());
const parts = (await eda.sch_PrimitiveComponent.get(await eda.sch_PrimitiveComponent.getAllPrimitiveId()))
  .filter((c) => c && c.getState_Designator && c.getState_Designator()).length;
return JSON.stringify({ res, stillThere: left, partsNow: parts, wiresNow: ((await eda.sch_PrimitiveWire.getAll()) || []).length });
