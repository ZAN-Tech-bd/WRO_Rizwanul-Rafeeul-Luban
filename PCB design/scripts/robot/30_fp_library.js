// Robot carrier v0.6 PCB — routing prep (pcb-pipeline template) (READ-ONLY): download the footprint file (elibz zip) of every distinct
// footprint on the board, base64, so the offline router knows the exact pad shapes, NPTH holes and slots.
// Uses the system-library footprint resolved from each part's LCSC id (project copies don't export).
// Time-boxed; run repeatedly with a different SKIP until "remaining": 0 (see `done` in the output).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\30_fp_library.js" > data\fp_lib_N.json
const SKIP = new Set(/*SKIP*/[]);
const PROJECT = 'WRO_Robot';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const byLcsc = {};
for (const c of comps) {
  const id = c.getState_SupplierId();
  (byLcsc[id] = byLcsc[id] || []).push(c.getState_Designator());
}
const t0 = Date.now();
const out = { parts: byLcsc, files: {}, remaining: [] };
for (const [lcsc, des] of Object.entries(byLcsc)) {
  if (SKIP.has(lcsc)) continue;
  if (Date.now() - t0 > 18000) { out.remaining.push(lcsc); continue; }
  try {
    const dev = [].concat((await eda.lib_Device.getByLcscIds([lcsc])) || [])[0];
    if (!dev) { out.files[lcsc] = { err: 'device not found' }; continue; }
    const f = await eda.sys_FileManager.getFootprintFileByFootprintUuid(dev.footprintUuid, dev.libraryUuid, 'elibz');
    const bytes = new Uint8Array(await f.arrayBuffer());
    let s = '';
    for (let i = 0; i < bytes.length; i += 8192) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 8192));
    out.files[lcsc] = { fp: dev.footprintName, des, b64: btoa(s) };
  } catch (e) { out.files[lcsc] = { err: String(e) }; }
}
return JSON.stringify(out);
