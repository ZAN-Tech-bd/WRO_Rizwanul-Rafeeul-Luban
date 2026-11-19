// WRITE Robot PCB1 ONLY: scoped +3V3 reroute out of the DevKit antenna overhang (D-037).
// Modifies exactly 3 existing LINE segments (net/layer kept, endpoints (470,3760)/(1765,3850)
// kept, width -> 0.8 mm class). No create/delete, no netlist/ECO, no library access.
// Offline gates: test_rf_repair.py (antenna >=4 mil, foreign copper >=6 mil, +3V3 connected).
// Canvas readback is NOT persistence evidence: a fresh full export + diff is mandatory.
const PLAN = __PLAN__;
const PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864';
const PCB = '643ce99542452c8f';
const p0 = await eda.dmt_Project.getCurrentProjectInfo();
if (p0?.uuid !== PROJECT || p0?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
const d0 = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d0?.documentType !== 3 || d0?.uuid !== PCB) throw new Error('Open Robot PCB1 (not a footprint tab)');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const near = (a, b) => Number.isFinite(a) && Number.isFinite(b) && Math.abs(a - b) < 0.0001;
const read = l => ({ id: l.getState_PrimitiveId(), net: String(l.getState_Net()),
  layer: l.getState_Layer(), startX: l.getState_StartX(), startY: l.getState_StartY(),
  endX: l.getState_EndX(), endY: l.getState_EndY(), width: l.getState_LineWidth() });
// 1) verify EVERY source segment exactly before the first write
const before = {};
for (const [rid, e] of Object.entries(PLAN.modify)) {
  const lines = await eda.pcb_PrimitiveLine.get([rid]);
  const l = Array.isArray(lines) ? lines[0] : lines;
  if (!l) throw new Error('Missing source line ' + rid);
  const b = read(l);
  const s = e.source;
  if (b.net !== s.net || b.layer !== s.layerId || !near(b.width, s.width) ||
      !near(b.startX, s.startX) || !near(b.startY, s.startY) ||
      !near(b.endX, s.endX) || !near(b.endY, s.endY))
    throw new Error('Source guard failed for ' + rid + ': ' + JSON.stringify(b));
  before[rid] = b;
}
// 2) apply the three modifications
for (const [rid, e] of Object.entries(PLAN.modify)) {
  const ok = await eda.pcb_PrimitiveLine.modify(rid, { startX: e.startX, startY: e.startY,
    endX: e.endX, endY: e.endY, lineWidth: e.lineWidth });
  if (!ok) throw new Error('modify returned falsy for ' + rid);
}
// 3) fresh readback of all three; net/layer unchanged, geometry = plan
const after = {};
for (const rid of Object.keys(PLAN.modify)) {
  const lines = await eda.pcb_PrimitiveLine.get([rid]);
  const l = Array.isArray(lines) ? lines[0] : lines;
  const a = read(l);
  const e = PLAN.modify[rid];
  if (a.net !== e.source.net || a.layer !== e.source.layerId ||
      !near(a.startX, e.startX) || !near(a.startY, e.startY) ||
      !near(a.endX, e.endX) || !near(a.endY, e.endY) || !near(a.width, e.lineWidth))
    throw new Error('Fresh readback failed for ' + rid + '; do not retry: ' + JSON.stringify(a));
  after[rid] = a;
}
if (!await eda.pcb_Document.save()) throw new Error('PCB save failed; do not retry');
return JSON.stringify({ pcb: PCB, modified: Object.keys(PLAN.modify).length, before, after,
  saveReturned: true,
  verification: 'REQUIRED: fresh full export + exact-change diff (only these 3 LINE bodies + header)' });
