// Robot carrier v0.6 — step P4j: swap J1–J4 to VERTICAL XT connectors (D-030) (WRITES the schematic).
// From the pcb-pipeline template schematic/17_swap_part.js. Same designators, same spot / rotation; deletes the old pins'
// stubs and the part, places the new device, then 14_wire.js re-creates the stubs from the (unchanged) net map
// (J1 1 = GND, 2 = BAT+; J2 / J3 1 = BAT_SW, 2 = GND; J4 1 = GND, 2 = +5V) and 15_check.js re-checks.
// Idempotent: a designator that already carries the new LCSC id is skipped. Gates: WRO_Robot, schematic tab, visible.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\17_swap_v06.js"
const SWAPS = [{ des: 'J1', lcsc: 'C2918078' }, { des: 'J2', lcsc: 'C108769' }, { des: 'J3', lcsc: 'C108769' }, { des: 'J4', lcsc: 'C428721' }];
const PROJECT = 'WRO_Robot';
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
// No timers: EasyEDA freezes its page timers while its window is hidden (setTimeout never fired, 2026-10-14),
// so 'sleep' yields through cheap API calls instead (those keep working while hidden).
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(500);
const res = [];
for (const { des, lcsc, newDes } of SWAPS) {
  const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
  const all = (await eda.sch_PrimitiveComponent.get(ids)).filter((c) => c && c.getState_Designator);
  const comps = all.filter((c) => c.getState_Designator() === des);
  if (newDes) {
    const done = all.filter((c) => c.getState_Designator() === newDes);
    if (!comps.length && done.length === 1 && done[0].getState_SupplierId && done[0].getState_SupplierId() === lcsc) { res.push({ des, already: newDes + ' ' + lcsc }); continue; }
    if (done.length) { res.push({ des, abort: newDes + ' already exists; nothing changed' }); continue; }
  }
  if (comps.length !== 1) { res.push({ des, abort: 'found ' + comps.length + ' parts with this designator' }); continue; }
  const c = comps[0];
  if (c.getState_SupplierId && c.getState_SupplierId() === lcsc) { res.push({ des, already: lcsc }); continue; }
  const dev = [].concat((await eda.lib_Device.getByLcscIds([lcsc])) || [])[0];
  if (!dev) { res.push({ des, abort: 'device not found ' + lcsc }); continue; }
  const x = c.getState_X(), y = c.getState_Y(), rot = c.getState_Rotation(), mir = c.getState_Mirror();
  const bom = c.getState_AddIntoBom(), old = c.getState_SupplierId();
  // stubs: wires with an end on one of the old pins
  const pins = (await c.getAllPins()) || [];
  const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
  const ends = (w) => { const f = (w.getState_Line() || []).flat(3); return [[f[0], f[1]], [f[f.length - 2], f[f.length - 1]]]; };
  const stubs = wires.filter((w) => ends(w).some(([ex, ey]) => pins.some((p) => Math.abs(ex - p.getState_X()) < 0.5 && Math.abs(ey - p.getState_Y()) < 0.5)));
  if (stubs.length) await eda.sch_PrimitiveWire.delete(stubs.map((w) => w.getState_PrimitiveId()));
  await eda.sch_PrimitiveComponent.delete([c.getState_PrimitiveId()]);
  await sleep(800);
  const n = await eda.sch_PrimitiveComponent.create({ libraryUuid: dev.libraryUuid, uuid: dev.uuid }, x, y, undefined, rot, mir, bom, true);
  if (!n) { res.push({ des, abort: 'create returned nothing (old part already deleted!)', x, y }); continue; }
  const a = n.toAsync(); a.setState_Designator(newDes || des); await a.done();
  await sleep(500);
  const np = (await n.getAllPins()) || [];
  res.push({ des, newDes: newDes || des, from: old, to: lcsc, uuid: dev.uuid, mpn: dev.manufacturerId, footprint: dev.footprintName, at: [x, y, rot],
    stubsRemoved: stubs.length, pins: np.map((p) => p.getState_PinNumber() + '@' + Math.round(p.getState_X()) + ',' + Math.round(p.getState_Y()) + ' r' + p.getState_Rotation()) });
}
await eda.sch_Document.save();
return JSON.stringify(res);
