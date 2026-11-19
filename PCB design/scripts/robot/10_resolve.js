// Robot carrier v0.4 — step P4a: resolve every BOM part to its EasyEDA library device (READ-ONLY).
// Output feeds 11_place.js (DEV map). Same LCSC list as 07_parts_verify_v04.js (THT gate PASS 2026-10-16).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\10_resolve.js"
const want = {
  XT60M: 'C98732', XT30M: 'C53373776', XT30F: 'C2913282', VH2: 'C485345',
  XH8: 'C157972', XH7: 'C144398', XH4: 'C144395', XH3: 'C144394', XH2: 'C378939',
  FH22: 'C46595983', FH8: 'C46595973', FH4: 'C541851', HDR1: 'C492400', HDR3: 'C492402', HDR4: 'C492403',
  D4007: 'C108355', D5819: 'C526214', TACT: 'C136680',
  R1R1: 'C1365792', R47: 'C3402507', R1K: 'C1364484', R2K2: 'C3454390', R4K7: 'C1371435',
  R10K: 'C1519255', R20K: 'C1364283', R100K: 'C1364475',
  C100N: 'C342794', C470U: 'C109066', C1000U16: 'C439782', C1000U35: 'C2960416', C10U: 'C503219',
};
const BY_NAME = { LED5: ['C9900102778', '5MM LED W-BLUE'] }; // library-only footprint (hub LED), found by name
const out = {};
let items = [];
try {
  const r = await eda.lib_Device.getByLcscIds(Object.values(want), undefined, true);
  items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
} catch (e) { out._err = String(e); }
const libs = new Set();
for (const [k, c] of Object.entries(want)) {
  const it = items.find((x) => x && x.supplierId === c);
  if (it) libs.add(it.libraryUuid);
  out[k] = it ? it.uuid : 'MISSING ' + c;
}
for (const [k, [sid, name]] of Object.entries(BY_NAME)) {
  const r = (await eda.lib_Device.search(name, undefined, undefined, undefined, 10, 1)) || [];
  const it = r.find((x) => x && x.supplierId === sid);
  if (it) libs.add(it.libraryUuid);
  out[k] = it ? it.uuid : 'MISSING ' + sid;
}
out._libs = [...libs];
return JSON.stringify(out);
