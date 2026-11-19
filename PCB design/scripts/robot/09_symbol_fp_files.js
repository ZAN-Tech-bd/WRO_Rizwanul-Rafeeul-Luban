// Robot carrier v0.4 — step P3d: polarity research (READ-ONLY). Downloads the system-library SYMBOL and FOOTPRINT
// files (elibz zips, base64) of every polarity-sensitive part so pin names / numbers and pad positions can be read
// offline before any wire is drawn (read_symbols.py, read_elibz.py). C99… ids are library-only parts: found by name.
// Run (twice is fine: ids already in the output file can be dropped from IDS):
//   node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\09_symbol_fp_files.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\data\sym_fp_files.json"
// batch 1 (done 2026-10-15 → data/sym_fp_files.json): C108355 C526214 C9900102778 C439782 C2960416
// batch 2 (done → data/sym_fp_files_b.json): C109066 C503219 C2913282 C53373776 C98732
// batch 3 (done → data/sym_fp_files_c.json): C136680 C485345
// batch 4 (v0.5 vertical XT, 2026-10-25 → data/sym_fp_files_xtv.json):
const IDS = ['C2918078', 'C108769', 'C428721'];
const BY_NAME = { C9900102778: '5MM LED W-BLUE' };
const toB64 = async (f) => {
  const bytes = new Uint8Array(await f.arrayBuffer());
  let s = '';
  for (let i = 0; i < bytes.length; i += 8192) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 8192));
  return btoa(s);
};
const sym = {}, fp = {};
const t0 = Date.now();
for (const id of IDS) {
  if (Date.now() - t0 > 19000) { sym[id] = fp[id] = { err: 'time box' }; continue; }
  try {
    let dev;
    if (BY_NAME[id]) {
      const r = (await eda.lib_Device.search(BY_NAME[id], undefined, undefined, undefined, 10, 1)) || [];
      dev = r.find((x) => x && x.supplierId === id);
    } else {
      dev = [].concat((await eda.lib_Device.getByLcscIds([id])) || [])[0];
    }
    if (!dev) { sym[id] = fp[id] = { err: 'not found' }; continue; }
    const o = dev.otherProperty || {};
    const symUuid = dev.symbolUuid || o.Symbol;
    const fpUuid = dev.footprintUuid || o.Footprint;
    const mpn = o['Manufacturer Part'] || dev.manufacturerId || dev.name;
    const sf = symUuid && await eda.sys_FileManager.getSymbolFileBySymbolUuid(symUuid, dev.libraryUuid, 'elibz');
    sym[id] = sf ? { mpn, sym: dev.symbolName, b64: await toB64(sf) } : { err: 'no symbol file', mpn };
    const ff = fpUuid && await eda.sys_FileManager.getFootprintFileByFootprintUuid(fpUuid, dev.libraryUuid, 'elibz');
    fp[id] = ff ? { mpn, fp: dev.footprintName, b64: await toB64(ff) } : { err: 'no footprint file', mpn, fp: dev.footprintName };
  } catch (e) { sym[id] = fp[id] = { err: String(e) }; }
}
return JSON.stringify({ sym, fp });
