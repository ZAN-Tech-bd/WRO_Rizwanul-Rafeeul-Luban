// Hub v0.4 — D-031 (2026-10-26): library SYMBOL + FOOTPRINT files of the vertical XT parts (READ-ONLY), so pin names,
// pad positions and drill sizes are read offline before the swap. Copy of robot/09_symbol_fp_files.js.
// C9900007500 (XT60UPB-F) is library-only: found by name.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\16_xt_vertical_files.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\data\sym_fp_files_xtv.json"
const IDS = ['C9900007500'];
const BY_NAME = { C9900007500: 'XT60UPB-F' };
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
