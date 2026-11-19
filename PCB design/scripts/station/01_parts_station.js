// Station board v0.1 — LCSC library search for its extra THT parts (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/station/01_parts_station.js"
const pick = (it) => it && ({ n: it.name, c: it.supplierId, fp: it.footprintName });
const q = async (key, n = 2) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  bc557: await q('BC557B'),
  bat85: await q('BAT85'),
  sk_1n5819: await q('1N5819'),
  r68k: await q('MFR-25FBF52-68K'),
  r100k: await q('MFR-25FBF52-100K'),
  fhdr3: await q('PM254V-11-03-H85'),
  fhdr4: await q('PM254V-11-04-H85'),
  buzzer_active: await q('active buzzer 12mm'),
}, null, 0);
