// Hub v0.3 — step 2c: retry searches by exact part names + THT passives (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/03_parts_passive.js"
const pick = (it) => it && ({
  n: it.name,
  c: it.supplierId,
  cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'],
  fp: it.footprintName,
  d: (it.description || '').slice(0, 80),
});
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  npn_2n2222a: await q('2N2222A'),
  npn_s8050: await q('S8050 TO-92'),
  buzzer: await q('Passive Buzzer'),
  led_red: await q('Red 5mm'),
  res_10k_axial: await q('10kΩ 1/4W axial'),
  cap_470u: await q('470uF 16V radial'),
  cap_100n_tht: await q('100nF 50V radial ceramic'),
  diode_1n4148: await q('1N4148 DO-35'),
}, null, 0);
