// Station board v0.3 — step P3a: verify + resolve every LCSC part of the BOM, THROUGH-HOLE GATE (READ-ONLY).
// Each footprint name is classified (SMD patterns fail, THT patterns pass, anything else = "review"); the library
// device uuids feed 11_place.js. Also searches the one value without a number yet (68 kΩ ¼ W axial).
// Pattern: robot 07_parts_verify_v04.js + hub 06_resolve_devices.js.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\02_parts_verify.js"
const want = {
  XH8: 'C157972', XH4: 'C144395', XH3: 'C144394', FH3: 'C541850',   // v0.4: FH4 (MQ-5 socket) gone, XH3 (DHT11 cable) in
  BC557: 'C2093', D5819: 'C526214', R1K: 'C1364484', R100K: 'C1364475',
  R68K: 'C385541',   // RN1/2WS 68 kΩ: same RES-TH P10.30 footprint as the MFR-25 parts (any ¼ W 68 kΩ fits)
};
const BY_NAME = { BUZ: ['C9900016920', 'buzzer-12X9'] };
const SMD = [/^(R|C|L|LED|D|F)\d{4}\b/i, /SMD/i, /^SO(D|T|IC|P)/i, /^SM[ABC]\b/i, /QFN|DFN|TSSOP|MSOP|SOT-?\d/i, /^SMA_/i, /LED\d{4}/i];
const THT = [/TH[_-]/i, /^DO-(15|41|201|204)/i, /^DIP/i, /TO-?(92|220)/i, /^HDR-TH/i, /BUZ/i];
const classify = (fp) => (!fp ? 'review' : SMD.some((re) => re.test(fp)) ? 'SMD' : THT.some((re) => re.test(fp)) ? 'THT' : 'review');
const out = { parts: {}, smd: [], review: [], missing: [], r68k: [] };
const row = (k, it, cls) => {
  const o = it.otherProperty || {};
  out.parts[k] = { c: it.supplierId, mpn: o['Manufacturer Part'] || it.manufacturerId || it.name, val: o['Value'] || '',
    fp: it.footprintName, cls, lib: it.libraryUuid, uuid: it.uuid };
  if (cls === 'SMD') out.smd.push(k);
  if (cls === 'review') out.review.push(k);
};
let items = [];
try {
  const r = await eda.lib_Device.getByLcscIds(Object.values(want), undefined, true);
  items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
} catch (e) { out._err = String(e); }
for (const [k, c] of Object.entries(want)) {
  const it = items.find((x) => x && x.supplierId === c);
  if (!it) { out.missing.push(k + ' ' + c); continue; }
  row(k, it, classify(it.footprintName));
}
for (const [k, [sid, name]] of Object.entries(BY_NAME)) {
  try {
    const r = (await eda.lib_Device.search(name, undefined, undefined, undefined, 10, 1)) || [];
    const it = r.find((x) => x && x.supplierId === sid);
    if (!it) { out.missing.push(k + ' ' + sid); continue; }
    row(k, it, classify(it.footprintName));
  } catch (e) { out.missing.push(k + ' ' + String(e)); }
}
// 68 kΩ ¼ W axial: same footprint family as the MFR-25 parts above
for (const key of ['MFR-25FBF52-68K', 'MFR-25FBF52-68K0', 'MFR25SFTE52-68K']) {
  try {
    const r = (await eda.lib_Device.search(key, undefined, undefined, undefined, 6, 1)) || [];
    for (const it of r) {
      if (!it) continue;
      const o = it.otherProperty || {};
      out.r68k.push({ c: it.supplierId, mpn: o['Manufacturer Part'] || it.name, val: o['Value'] || '', fp: it.footprintName,
        cls: classify(it.footprintName), lib: it.libraryUuid, uuid: it.uuid });
    }
  } catch (e) { out.r68k.push({ key, err: String(e) }); }
}
out.gate = out.smd.length === 0 && out.missing.length === 0 ? (out.review.length ? 'REVIEW' : 'PASS') : 'FAIL';
return JSON.stringify(out);
