// Robot carrier v0.3 — step 2e: diodes, new resistor values, electrolytics (READ-ONLY).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/robot/05_parts_v03_passives.js"
const pick = (it) => it && ({ n: it.name, c: it.supplierId, cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'], fp: it.footprintName, d: it.description });
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  d_1n4007: await q('1N4007 DO-41', 3),
  d_1n5822: await q('1N5822 DO-201AD', 3),
  r_1r_1w: await q('1Ω 1W axial', 3),
  r_47r: await q('MFR-25FBF52-47R', 2),
  r_100k: await q('MFR-25FBF52-100K', 2),
  c_1000u_25v: await q('1000uF 25V radial', 3),
  c_1000u_16v: await q('1000uF 16V radial', 3),
  c_10u_25v: await q('10uF 25V radial', 3),
}, null, 0);
