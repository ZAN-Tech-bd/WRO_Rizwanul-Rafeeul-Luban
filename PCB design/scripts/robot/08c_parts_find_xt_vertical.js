// Robot carrier v0.5 — step P3c: find VERTICAL (top-entry) XT60 / XT30 board connectors (READ-ONLY).
// User 2026-10-25: the team's XT connectors are the normal vertical ones, not the right-angle XT60PW / XT30PW.
// Keeps only through-hole footprints and prints MPN, footprint, class. Time-boxed (~18 s).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\08c_parts_find_xt_vertical.js"
const SYS = '0819f05c4eef4c71ace90d822a990e87';
const t0 = Date.now();
const SMD = /SMD|^(R|C|LED)\d{4}|^SO[DTP]|^SM[ABC]\b|LED\d{4}/i;
const pick = (it) => {
  const o = it.otherProperty || {};
  return [it.supplierId, o['Manufacturer Part'] || it.manufacturerId || it.name, o['Manufacturer'] || '', o['JLCPCB Part Class'] || '', it.footprintName];
};
const q = async (key, n, re) => {
  if (Date.now() - t0 > 17000) return 'SKIPPED (time box)';
  try {
    const r = (await eda.lib_Device.search(key, SYS, undefined, undefined, n, 1)) || [];
    return r.filter((it) => it && !SMD.test(it.footprintName || '') && (!re || re.test((it.footprintName || '') + ' ' + (it.name || '') + ' ' + JSON.stringify(it.otherProperty || {})))).map(pick).slice(0, 8);
  } catch (e) { return 'ERR ' + e; }
};
return JSON.stringify({
  xt60m: await q('XT60-M', 12, /XT60/i),
  xt60u: await q('XT60U', 12, /XT60/i),
  xt30m: await q('XT30UPB-M', 12, /XT30/i),
  xt30f: await q('XT30UPB-F', 12, /XT30/i),
  xt30any: await q('XT30U', 12, /XT30/i),
});
