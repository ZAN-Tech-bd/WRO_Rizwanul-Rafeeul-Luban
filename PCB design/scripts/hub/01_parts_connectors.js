// Hub v0.3 — step 2a: LCSC library search for the through-hole connectors (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/01_parts_connectors.js"
const pick = (it) => it && ({
  n: it.name,
  c: it.supplierId,
  cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'],
  fp: it.footprintName,
  d: it.description,
});
const q = async (key, n = 4) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  xt30pw_m: await q('XT30PW-M'),
  vh_6p: await q('B6P-VH'),
  xh_8p: await q('B8B-XH-A'),
  xh_4p: await q('B4B-XH-A'),
  xh_3p: await q('B3B-XH-A(LF)(SN)'),
  fhdr_22: await q('PM254V-11-22-H85'),
  fhdr_06: await q('PM254V-11-06-H85'),
}, null, 0);
