// Hub v0.3 — step 2e: remaining axial resistor values (MFR-25 series) + DO-35 1N4148 (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/05_parts_values.js"
const pick = (it) => it && ({ n: it.name, c: it.supplierId, fp: it.footprintName });
const q1 = async (key) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, 2, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
const out = {};
for (const v of ['1K', '20K', '330R', '220R', '2K2', '4K7']) out['mfr25_' + v] = await q1('MFR-25FBF52-' + v);
out.d1n4148_tr = await q1('1N4148TR');
out.d1n4148_ta = await q1('1N4148TA');
return JSON.stringify(out, null, 0);
