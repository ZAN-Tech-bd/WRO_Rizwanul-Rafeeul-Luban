// Robot carrier v0.4 — step P3b: search the BOM gaps, keep only THROUGH-HOLE footprints (READ-ONLY).
// Gaps: XT30 female board socket, 1x3 / 1x4 male header, 6x6 THT tact switch, radial electrolytics
// (1000 uF 16 V, 1000 uF 25 V, 10 uF), 5 mm LED footprint. Time-boxed (~18 s).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\08_parts_find_v04.js"
const SYS = '0819f05c4eef4c71ace90d822a990e87';
const t0 = Date.now();
const SMD = /SMD|^(R|C|LED)\d{4}|^SO[DTP]|^SM[ABC]\b|LED\d{4}/i;
const pick = (it) => {
  const o = it.otherProperty || {};
  return [it.supplierId, o['Manufacturer Part'] || it.manufacturerId || it.name, o['Value'] || '', o['JLCPCB Part Class'] || '', it.footprintName];
};
const q = async (key, n, re) => {
  if (Date.now() - t0 > 17000) return 'SKIPPED (time box)';
  try {
    const r = (await eda.lib_Device.search(key, SYS, undefined, undefined, n, 1)) || [];
    return r.filter((it) => it && !SMD.test(it.footprintName || '') && (!re || re.test((it.footprintName || '') + ' ' + (it.name || '') + ' ' + JSON.stringify(it.otherProperty || {})))).map(pick).slice(0, 4);
  } catch (e) { return 'ERR ' + e; }
};
return JSON.stringify({
  xt30f: await q('XT30PW-F', 6, /XT30/i),
  hdr3: await q('PZ254V-11-03P', 4, /HDR-TH|TH/i),
  tact_a: await q('TS-1102-C-W', 6, /TH/i),
  tact_b: await q('tactile switch 6x6 DIP', 10, /TH/i),
  tact_c: await q('K2-1102DP', 6, /TH/i),
  cap1000_16: await q('1000uF 16V', 12, /CAP-TH/i),
  cap1000_25: await q('1000uF 25V', 12, /CAP-TH/i),
  cap10u: await q('10uF 25V electrolytic', 12, /CAP-TH/i),
  led5: await q('5MM LED', 6, /LED/i),
});
