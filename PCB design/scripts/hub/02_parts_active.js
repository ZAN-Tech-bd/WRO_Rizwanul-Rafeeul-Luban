// Hub v0.3 — step 2b: LCSC library search for THT semiconductors, buzzer, LEDs (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/02_parts_active.js"
const pick = (it) => it && ({
  n: it.name,
  c: it.supplierId,
  cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'],
  fp: it.footprintName,
  d: (it.description || '').slice(0, 90),
});
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  xh_4p: await q('B4B-XH-A(LF)(SN)'),
  hct125_dip: await q('74HCT125N'),
  dip14_socket: await q('DIP-14 IC socket'),
  npn_2n2222: await q('2N2222A TO-92'),
  buzzer_passive: await q('passive buzzer 12mm THT'),
  led5_red: await q('LED 5mm red THT'),
  led5_blue: await q('LED 5mm blue THT'),
  led5_green: await q('LED 5mm green THT'),
}, null, 0);
