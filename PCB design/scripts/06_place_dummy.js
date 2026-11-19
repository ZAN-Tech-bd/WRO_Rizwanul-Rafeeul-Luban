// Step 3 — PLACE dummy-board parts on the active schematic (Schematic1 must be open).
// Parts chosen in 05_pick_parts.js: C2286 red LED 0603 (Basic), C21190 1k 0603 (Basic),
// C9900016709 female header 1x3 2.54mm (Extended).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/06_place_dummy.js"
const LIB = '0819f05c4eef4c71ace90d822a990e87'; // LCSC system library
const LED = { libraryUuid: LIB, uuid: '015cbd77079d4b0ea5c81b79fefdcea3' }; // C2286
const R1K = { libraryUuid: LIB, uuid: '2340647df7144fc5a30489ed06518eb5' }; // C21190
const HDR = { libraryUuid: LIB, uuid: 'aac61ba5a4e84f2799a767bfeb452c1e' }; // C9900016709

const summary = {};
const place = async (tag, part, x, y, rot) => {
  try {
    const r = await eda.sch_PrimitiveComponent.create(part, x, y, undefined, rot || 0);
    summary[tag] = r && r.id ? { ok: true, id: r.id } : { ok: true, note: 'placed (no id returned)' };
  } catch (e) {
    summary[tag] = { ok: false, error: String(e) };
  }
};

await place('HDR1 (J1)', HDR, 400, 500);
await place('R1', R1K, 700, 400);
await place('LED1', LED, 950, 400);
await place('R2', R1K, 700, 700);
await place('LED2', LED, 950, 700);

try {
  await eda.dmt_EditorControl.zoomToAllPrimitives();
} catch (e) {
  summary.zoom = String(e);
}

try {
  await eda.sch_Document.save();
  summary.saved = true;
} catch (e) {
  summary.save = String(e);
}

return JSON.stringify(summary, null, 1);
