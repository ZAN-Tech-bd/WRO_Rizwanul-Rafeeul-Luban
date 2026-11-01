// Robot carrier v0.3 — step 2f: exact-MPN lookups for the parts 05 could not pin down (READ-ONLY).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/robot/06_parts_v03_exact.js"
const pick = (it) => it && ({ n: it.name, c: it.supplierId, cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'], fp: it.footprintName, d: it.description && it.description.slice(0, 140) });
const q = async (key, n = 2) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  d_1n4007_g: await q('1N4007G', 2),
  d_1n4007_t: await q('1N4007-T', 2),
  d_1n5822_t: await q('1N5822-T', 2),
  d_1n5819_t: await q('1N5819-T', 2),
  r_1r_cfr: await q('CFR-100JB-52-1R', 2),
  r_1r_rsf: await q('RSF100JB-73-1R', 2),
  c_1000u25: await q('RVT1E102M1225', 2),
  c_1000u16: await q('RVT1C102M1020', 2),
  tact_vert: await q('TS-1102S-6x6x5', 2),
  tact_vert2: await q('K2-1102SP-C4SC-04', 2),
}, null, 0);
