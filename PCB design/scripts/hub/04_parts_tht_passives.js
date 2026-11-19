// Hub v0.3 — step 2d: through-hole passives by exact manufacturer part number (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/04_parts_tht_passives.js"
const pick = (it) => it && ({
  n: it.name,
  c: it.supplierId,
  cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'],
  fp: it.footprintName,
});
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  res_mfr25_10k: await q('MFR-25FBF52-10K'),
  res_cfr25_10k: await q('CFR-25JB-52-10K'),
  cap_100n_fg28: await q('FG28X7R1H104KNT06'),
  cap_100n_murata: await q('RDER71H104K0P1H03B'),
  cap_470u_rubycon: await q('16ZLH470MEFC8X11.5'),
  cap_470u_generic: await q('470uF 16V 8x12mm THT'),
  diode_1n4148: await q('1N4148'),
}, null, 0);
