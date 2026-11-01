// Robot carrier v0.4 — step P3c: second gap search (READ-ONLY): VAMP capacitor (1000 uF >= 25 V radial),
// RFID 10 uF radial, 5 mm THT LEDs (red, green), and the 470 uF / XT30 female pin data. Time-boxed (~18 s).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\08b_parts_find_v04.js"
const SYS = '0819f05c4eef4c71ace90d822a990e87';
const t0 = Date.now();
const SMD = /SMD|^(R|C|LED)\d{4}|^SO[DTP]|^SM[ABC]\b|LED\d{4}/i;
const pick = (it) => {
  const o = it.otherProperty || {};
  return [it.supplierId, o['Manufacturer Part'] || it.manufacturerId || it.name, o['Value'] || '', it.footprintName];
};
const q = async (key, n, re) => {
  if (Date.now() - t0 > 17000) return 'SKIPPED (time box)';
  try {
    const r = (await eda.lib_Device.search(key, SYS, undefined, undefined, n, 1)) || [];
    return r.filter((it) => it && /^C\d+$/.test(it.supplierId || '') && !/^C99/.test(it.supplierId) && !SMD.test(it.footprintName || '') &&
      (!re || re.test((it.footprintName || '') + ' ' + JSON.stringify(it.otherProperty || {})))).map(pick).slice(0, 5);
  } catch (e) { return 'ERR ' + e; }
};
return JSON.stringify({
  c1000_25a: await q('1000uF 25V', 20, /CAP-TH/i),
  c1000_25b: await q('1mF 25V', 20, /CAP-TH/i),
  c1000_35: await q('1000uF 35V', 20, /CAP-TH/i),
  c10u_a: await q('10uF 50V', 20, /CAP-TH/i),
  c10u_b: await q('10uF 25V', 20, /CAP-TH/i),
  led_red: await q('5mm red LED', 15, /LED-TH|TH/i),
  led_green: await q('5mm green LED', 15, /LED-TH|TH/i),
});
