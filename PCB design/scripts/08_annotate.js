// Step 5 — ANNOTATE designators (U? R? LED? → J1/R1/R2/LED1/LED2), drop the phantom
// template primitive, save. setState_Designator works in sync mode (comp.toSync()).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/08_annotate.js"
const log = {};
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);

const real = [];
for (const c of comps) {
  const pins = (await c.getAllPins()) || [];
  real.push({ c, pins, cur: c.getState_Designator() });
}

const headers = real.filter((r) => r.pins.length === 3);
const resistors = real
  .filter((r) => /^R\?*$/.test(String(r.cur)) && r.pins.length === 2)
  .sort((a, b) => a.pins[0].getState_Y() - b.pins[0].getState_Y());
const leds = real
  .filter((r) => /^(LED|D)\?*$/.test(String(r.cur)) && r.pins.length === 2)
  .sort((a, b) => a.pins[0].getState_Y() - b.pins[0].getState_Y());

const assign = [
  [headers[0], 'J1'],
  [resistors[0], 'R1'],
  [resistors[1], 'R2'],
  [leds[0], 'LED1'],
  [leds[1], 'LED2'],
];

log.assigned = [];
for (const [r, des] of assign) {
  if (!r) continue;
  try {
    r.c.toSync().setState_Designator(des);
    log.assigned.push({ des, ok: true, was: r.cur });
  } catch (e) {
    log.assigned.push({ des, ok: false, error: String(e) });
  }
}

// phantom "?" primitives (0 pins) — try to delete
log.deleted = [];
for (const r of real.filter((x) => x.pins.length === 0)) {
  try {
    await eda.sch_PrimitiveComponent.delete([r.c.getState_PrimitiveId()]);
    log.deleted.push({ ok: true });
  } catch (e) {
    log.deleted.push({ ok: false, error: String(e) });
  }
}

await eda.sch_Document.save();

// verify: re-read designators
const ids2 = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps2 = await eda.sch_PrimitiveComponent.get(ids2);
log.designatorsAfter = [];
for (const c of comps2) {
  log.designatorsAfter.push(
    (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?'
  );
}

return JSON.stringify(log, null, 1);
