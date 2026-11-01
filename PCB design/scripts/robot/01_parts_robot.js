// Robot carrier v0.1 — step 2a: LCSC library search (READ-ONLY, like scripts/hub/01).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/robot/01_parts_robot.js"
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
  xt60_m: await q('XT60PW-M', 2),
  xt60_f: await q('XT60PW-F', 2),
  xt30_m: await q('XT30PW-M', 2),
  vh_6p: await q('B6P-VH', 2),
  xh_2p: await q('B2B-XH-A', 2),
  xh_3p: await q('B3B-XH-A(LF)(SN)', 2),
  xh_4p: await q('B4B-XH-A', 2),
  xh_8p: await q('B8B-XH-A', 2),
  fhdr_15: await q('PM254V-11-15-H85', 3),
  fhdr_08: await q('PM254V-11-08-H85', 3),
  fhdr_07: await q('PM254V-11-07-H85', 3),
  fhdr_02: await q('PM254V-11-02-H85', 3),
  screwterm_2p: await q('DG301-5.08-02P', 3),
  dfplayer_fp: await q('DFPlayer Mini', 3),
  r20k_axial: await q('20kΩ ±1% 250mW', 4),
  dip14_socket: await q('14P-2.54 DIP socket', 3),
}, null, 0);
