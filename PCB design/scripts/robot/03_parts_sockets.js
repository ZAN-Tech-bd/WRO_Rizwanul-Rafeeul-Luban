// Robot carrier v0.2 — step 2c: sockets for the S3 DevKit + LM386 module (READ-ONLY).
const pick = (it) => it && ({ n: it.name, c: it.supplierId, cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'], fp: it.footprintName, d: it.description });
const q = async (key, n = 3) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  fhdr_22: await q('PM254V-11-22-H85', 2),
  fhdr_03: await q('PM254V-11-03-H85', 2),
  xh_2p: await q('B2B-XH-A-BK(LF)(SN)', 2),
}, null, 0);
