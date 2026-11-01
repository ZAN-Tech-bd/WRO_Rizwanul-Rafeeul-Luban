// Robot carrier v0.3 — step 2d: connectors + sockets for the v0.3 port list (READ-ONLY).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/robot/04_parts_v03_connectors.js"
const pick = (it) => it && ({ n: it.name, c: it.supplierId, cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'], fp: it.footprintName, d: it.description });
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  xh_5p: await q('B5B-XH-A(LF)(SN)', 2),
  xh_7p: await q('B7B-XH-A(LF)(SN)', 2),
  xh_4p: await q('B4B-XH-A(LF)(SN)', 2),
  vh_2p: await q('B2P-VH', 3),
  fhdr_04: await q('PM254V-11-04-H85', 2),
  hdr_1x4_male: await q('PZ254V-11-04P', 2),
  tact_6x6: await q('TS-1102S', 3),
  tact_6x6b: await q('tactile switch 6x6 THT', 3),
}, null, 0);
