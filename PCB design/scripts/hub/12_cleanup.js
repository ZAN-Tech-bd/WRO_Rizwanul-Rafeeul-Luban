// Hub v0.3 — ERC cleanup after the user's re-arrangement (2026-10-12):
//  (a) delete wires carrying their net name twice (a create that "failed" but committed, then retried),
//      plus unnamed stray wires and the one-pin S8_DOUT stub; 09_wire_nets.js redraws the stubs once;
//  (b) mark every intentionally unconnected pin as No Connect (clears the "pins floating" warning).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/12_cleanup.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const NC = {
  J1: ['3', '4'], J2: ['3', '4'], J4: ['1', '2'], J5: ['3', '4'], U1A: ['3'],
  U1B: ['2', '3', '11', '12', '13', '14', '19', '20'], U2: ['8', '11'], J17: ['4', '5', '8'],
};
const res = { deleted: 0, ncSet: 0, errors: [] };
const doomed = [];
for (const w of (await eda.sch_PrimitiveWire.getAll()) || []) {
  const id = w.getState_PrimitiveId();
  const names = ((await eda.sch_PrimitiveAttribute.getAll(id)) || []).filter((a) => a.getState_Key() === 'Name');
  const net = w.getState_Net() || '';
  if (names.length > 1 || !net || net === 'S8_DOUT') doomed.push(id);
}
if (doomed.length) {
  try { await eda.sch_PrimitiveWire.delete(doomed); res.deleted = doomed.length; } catch (e) { res.errors.push('delete ' + e); }
}
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!NC[des]) continue;
  for (const p of (await c.getAllPins()) || []) {
    if (!NC[des].includes(String(p.getState_PinNumber()))) continue;
    try {
      if (!p.getState_NoConnected()) { const a = p.toAsync(); a.setState_NoConnected(true); await a.done(); }
      res.ncSet++;
    } catch (e) { res.errors.push(des + '.' + p.getState_PinNumber() + ' ' + e); }
  }
}
await sleep(2500);
res.wiresNow = (await eda.sch_PrimitiveWire.getAllPrimitiveId()).length;
return JSON.stringify(res);
