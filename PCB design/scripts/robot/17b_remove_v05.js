// Robot carrier v0.5 — step P4h: remove placed schematic parts for good (WRITES the schematic).
// v0.5 (2026-10-25, user-approved part-count review): C1 + C2 (no capacitor at J4, D-024), D3 (Mega fed straight from
// +5V, D-022), R1 (VAMP series resistor, D-023), TP1–TP6 (test points, D-026). Designators retired (spec v0.5 header).
// Per part: deletes the wire stubs that end on its pins (14_wire.js gives every pin its own named stub, so no other
// wire depends on them), then the part itself, then saves. 14_wire.js / 11_place.js no longer list these parts, so a
// later re-run can't bring them back. Then: 16_relabel_v05.js, 15_check.js + sch_audit.py, and the user imports the
// change into the PCB (Import Changes -> Apply).
// Idempotent: a designator that is already gone is reported as "gone" and nothing is changed. Time-boxed (≤ 18 s).
// From the pcb-pipeline template schematic/17b_remove_part.js. Gates: WRO_Robot, schematic tab, window visible.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\17b_remove_v05.js"
const REMOVE = ['C1', 'C2', 'D3', 'R1', 'TP1', 'TP2', 'TP3', 'TP4', 'TP5', 'TP6'];
const PROJECT = 'WRO_Robot';
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
