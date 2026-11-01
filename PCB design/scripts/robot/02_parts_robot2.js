// Robot carrier v0.1 — step 2b: fill gaps from 01 (READ-ONLY).
const pick = (it) => it && ({ n: it.name, c: it.supplierId, cls: it.otherProperty && it.otherProperty['JLCPCB Part Class'], fp: it.footprintName, d: it.description });
const q = async (key, n = 4) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, n, 1);
    const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
    return items.map(pick);
  } catch (e) { return String(e); }
};
return JSON.stringify({
  r20k_mfr: await q('MFR-25FBF52-20K', 3),
  r20k_tht: await q('20KΩ 250mW', 4),
  screw_508_2p: await q('DG301-5.08-02P', 3),
  screw_508: await q('screw terminal 5.08mm 2P', 4),
  r10k_axial_check: await q('MFR-25FBF52-10K', 2),
}, null, 0);
