// Robot carrier v0.4 — step P3a: verify every LCSC part of the v0.4 BOM + THROUGH-HOLE GATE (READ-ONLY).
// User rule (2026-10-15): every part on this PCB is through-hole, never SMD. Each footprint name is classified:
// SMD patterns fail the gate, THT patterns pass, anything else is listed under "review".
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\07_parts_verify_v04.js"
// Final v0.4 list (2026-10-16). Notes from 08/08b: C431092 is an XT30 MALE (XT30PW-M30.G.Y), not the female →
// J2/J3 = C2913282 XT30PW-F. Tact = C136680 (vertical THT 6x6) instead of the right-angle C54123281.
// LED = library footprint C9900102778 (LED-TH_BD5.6-P2.54, same as the hub), looked up by name below.
const want = {
  // connectors (J1–J13)
  XT60M: 'C98732', XT30M: 'C53373776', XT30F: 'C2913282', VH2: 'C485345',
  XH8: 'C157972', XH7: 'C144398', XH4: 'C144395', XH3: 'C144394', XH2: 'C378939',
  // sockets, headers, pads-only parts
  FH22: 'C46595983', FH8: 'C46595973', FH4: 'C541851', HDR1: 'C492400', HDR3: 'C492402', HDR4: 'C492403',
  // semiconductors, switches
  D4007: 'C108355', D5819: 'C526214', TACT: 'C136680',
  // resistors (Yageo MFR-25 1/4 W + RSF100 1 W)
  R1R1: 'C1365792', R47: 'C3402507', R1K: 'C1364484', R2K2: 'C3454390', R4K7: 'C1371435',
  R10K: 'C1519255', R20K: 'C1364283', R100K: 'C1364475',
  // capacitors
  C100N: 'C342794', C470U: 'C109066', C1000U16: 'C439782', C1000U35: 'C2960416', C10U: 'C503219',
};
const BY_NAME = { LED5: ['C9900102778', '5MM LED W-BLUE'] };
const SMD = [/^(R|C|L|LED|D|F)\d{4}\b/i, /SMD/i, /^SO(D|T|IC|P)/i, /^SM[ABC]\b/i, /QFN|DFN|TSSOP|MSOP|SOT-?\d/i, /^SMA_/i, /LED\d{4}/i];
const THT = [/TH[_-]/i, /^DO-(15|41|201|204)/i, /^DIP/i, /TO-(92|220)/i, /XT(30|60)/i, /^HDR-TH/i];
const classify = (fp) => {
  if (!fp) return 'review';
  if (SMD.some((re) => re.test(fp))) return 'SMD';
  if (THT.some((re) => re.test(fp))) return 'THT';
  return 'review';
};
const out = { parts: {}, smd: [], review: [], missing: [] };
let items = [];
try {
  const r = await eda.lib_Device.getByLcscIds(Object.values(want), undefined, true);
  items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
} catch (e) { out._err = String(e); }
for (const [k, c] of Object.entries(want)) {
  const it = items.find((x) => x && x.supplierId === c);
  if (!it) { out.missing.push(k + ' ' + c); continue; }
  const o = it.otherProperty || {};
  const cls = classify(it.footprintName);
  out.parts[k] = [c, o['Manufacturer Part'] || it.manufacturerId || it.name, o['Value'] || '', o['JLCPCB Part Class'] || '', it.footprintName, cls];
  if (cls === 'SMD') out.smd.push(k);
  if (cls === 'review') out.review.push(k);
}
for (const [k, [sid, name]] of Object.entries(BY_NAME)) {
  try {
    const r = (await eda.lib_Device.search(name, undefined, undefined, undefined, 10, 1)) || [];
    const it = r.find((x) => x && x.supplierId === sid);
    if (!it) { out.missing.push(k + ' ' + sid); continue; }
    const cls = classify(it.footprintName);
    out.parts[k] = [sid, it.name, '', 'library footprint (buy locally)', it.footprintName, cls];
    if (cls === 'SMD') out.smd.push(k);
    if (cls === 'review') out.review.push(k);
  } catch (e) { out.missing.push(k + ' ' + String(e)); }
}
out.count = Object.keys(out.parts).length;
out.gate = out.smd.length === 0 && out.missing.length === 0 ? (out.review.length ? 'REVIEW' : 'PASS') : 'FAIL';
return JSON.stringify(out, null, 0);
