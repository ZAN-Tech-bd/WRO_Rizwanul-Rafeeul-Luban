// Hub v0.3 — step 3a: resolve every BOM part to its EasyEDA library device (read-only).
// Output feeds 07_place.js. Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/06_resolve_devices.js"
const want = {
  XT60M: 'C2918078', XT60F: 'C9900007500', XT30M: 'C428721',   // D-031: vertical XT (was C98732 / C428722 / C53373776)
  VH6: 'C20088586',
  XH8: 'C157972', XH4: 'C144395', XH3: 'C144394', FH22: 'C46595983', FH6: 'C2832269',
  HCT125: 'C154102', NPN: 'C50229519', D4148: 'C84412',
  R10K: 'C1519255', R1K: 'C1364484', R220: 'C3537012', R330: 'C1369900', R2K2: 'C3454390', R4K7: 'C1371435',
  C100N: 'C342794', C470U: 'C109066',
};
const out = {};
let items = [];
try {
  const r = await eda.lib_Device.getByLcscIds(Object.values(want), undefined, true);
  items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
} catch (e) { out._err = String(e); }
for (const [k, c] of Object.entries(want)) {
  const it = items.find((x) => x && x.supplierId === c);
  out[k] = it ? { lib: it.libraryUuid, uuid: it.uuid, fp: it.footprintName } : null;
}
// user-library footprints (not real LCSC stock): find by name
const byName = async (key, name, sid) => {
  try {
    const r = await eda.lib_Device.search(name, undefined, undefined, undefined, 10, 1);
    const arr = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    const it = arr.find((x) => x && x.supplierId === sid) || null;
    out[key] = it ? { lib: it.libraryUuid, uuid: it.uuid, fp: it.footprintName } : null;
  } catch (e) { out[key] = String(e); }
};
await byName('BUZ', 'buzzer-12X9', 'C9900016920');
await byName('LED5', '5MM LED W-BLUE', 'C9900102778');
// candidates for solder jumpers and test points
const peek = async (key, name) => {
  try {
    const r = await eda.lib_Device.search(name, undefined, undefined, undefined, 4, 1);
    const arr = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    out[key] = arr.map((x) => x && { n: x.name, c: x.supplierId, fp: x.footprintName, lib: x.libraryUuid, uuid: x.uuid });
  } catch (e) { out[key] = String(e); }
};
await peek('cand_SJ', 'SolderJumper 2 Open');
await peek('cand_TP', 'TestPoint THT');
return JSON.stringify(out, null, 0);
