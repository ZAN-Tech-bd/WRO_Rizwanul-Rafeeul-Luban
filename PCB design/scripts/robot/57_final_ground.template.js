// WRITE Robot PCB1 ONLY: create the validated GND fills (test_ground_plan.py gates).
// Source-locked to the exact saved snapshot (components+pads, tracks, vias): any drift aborts
// before a write. Idempotent keep-matching; 14.5 s batches; save each run. Hub-66 method.
// NO component/route/via/net/footprint/outline edits. Full-export verification required after.
const CONFIG = __CONFIG__;
const fail = m => { throw new Error(m); };
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.uuid !== CONFIG.project || project?.friendlyName !== 'WRO_Robot') fail('Wrong project');
if (doc?.uuid !== CONFIG.pcb || doc?.documentType !== 3) fail('Open Robot PCB1');
if (document.visibilityState !== 'visible') fail('Keep EasyEDA visible');
const near = (a, b) => Math.abs(a - b) < .002;
const nearSize = (a, b) => Math.abs(a - b) < .11;   // live-getter unit rounding (~0.05 mil)
const nearXY = (a, b) => Math.abs(a - b) < .11;     // live getters round coords to 0.1 mil
const components = [];
for (const c of await eda.pcb_PrimitiveComponent.getAll()) {
  const pads = (await c.getAllPins()).map(p => ({ id: p.getState_PrimitiveId(),
    number: p.getState_PadNumber(), net: p.getState_Net(), x: p.getState_X(), y: p.getState_Y() }))
    .sort((a, b) => String(a.number).localeCompare(String(b.number)));
  components.push({ id: c.getState_PrimitiveId(), designator: c.getState_Designator(),
    x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(),
    footprint: c.getState_Footprint(), pads });
}
const normDeg = r => ((Number(r) % 360) + 360) % 360;  // live getter may say -90 for 270
components.sort((a, b) => a.designator.localeCompare(b.designator));
const snap = CONFIG.components.map(c => ({ ...c,
  pads: c.pads.map(p => ({ ...p, x: p.x + c.x, y: p.y + c.y })) }));
if (components.length !== snap.length) fail('Component count changed; re-plan');
for (let k = 0; k < components.length; k++) {
  const a = components[k], b = snap[k];
  if (a.designator !== b.designator) fail('Component identity/order changed: ' + a.designator);
  if (!nearXY(a.x, b.x) || !nearXY(a.y, b.y) ||
      Math.abs(normDeg(a.rotation) - normDeg(b.rotation)) > 0.001)
    fail('Component moved/rotated: ' + a.designator);
  const fpu = a.footprint && (a.footprint.uuid ?? a.footprint);
  if (fpu !== b.footprint) fail('Footprint binding changed: ' + a.designator);
  if (a.pads.length !== b.pads.length) fail('Pad count changed: ' + a.designator);
  for (let n = 0; n < a.pads.length; n++) {
    const p = a.pads[n], q = b.pads[n];
    if (String(p.number) !== String(q.number) || String(p.net ?? '') !== String(q.net))
      fail('Pad number/net changed: ' + a.designator + '.' + p.number);
    if (!nearXY(p.x, q.x) || !nearXY(p.y, q.y))
      fail('Pad moved: ' + a.designator + '.' + p.number);
  }
}
const lines = (await eda.pcb_PrimitiveLine.getAll()).filter(p => [1, 2].includes(Number(p.getState_Layer())));
if (lines.length !== CONFIG.lines.length) fail('Track count changed');
const byLine = new Map(CONFIG.lines.map(p => [p.id, p]));
for (const p of lines) {
  const b = byLine.get(p.getState_PrimitiveId());
  if (!b || p.getState_Net() !== b.net || Number(p.getState_Layer()) !== b.layer ||
      !nearXY(p.getState_StartX(), b.x1) || !nearXY(p.getState_StartY(), b.y1) ||
      !nearXY(p.getState_EndX(), b.x2) || !nearXY(p.getState_EndY(), b.y2) ||
      !nearSize(p.getState_LineWidth(), b.width)) fail('Track geometry changed');
}
const vias = await eda.pcb_PrimitiveVia.getAll();
if (vias.length !== CONFIG.vias.length) fail('Via count changed');
const byVia = new Map(CONFIG.vias.map(p => [p.id, p]));
for (const p of vias) {
  const b = byVia.get(p.getState_PrimitiveId());
  if (!b || p.getState_Net() !== b.net || !nearXY(p.getState_X(), b.x) || !nearXY(p.getState_Y(), b.y) ||
      !nearSize(p.getState_HoleDiameter(), b.hole) || !nearSize(p.getState_Diameter(), b.diameter))
    fail('Via identity changed');
}
const points = src => {
  const a = (Array.isArray(src[0]) ? src[0] : src).filter(x => typeof x === 'number');
  const pts = [];
  for (let i = 0; i < a.length; i += 2) pts.push([a[i], a[i + 1]]);
  if (pts.length > 1 && near(pts[0][0], pts[pts.length - 1][0]) && near(pts[0][1], pts[pts.length - 1][1])) pts.pop();
  return pts;
};
const segmentDistance = (p, a, b) => { const x = b[0] - a[0], y = b[1] - a[1], den = x * x + y * y;
  const t = den ? Math.max(0, Math.min(1, ((p[0] - a[0]) * x + (p[1] - a[1]) * y) / den)) : 0;
  return Math.hypot(p[0] - a[0] - t * x, p[1] - a[1] - t * y); };
const sameOutline = (a, b) => a.every(p => b.some((q, i) => segmentDistance(p, q, b[(i + 1) % b.length]) < .03)) &&
  b.every(p => a.some((q, i) => segmentDistance(p, q, a[(i + 1) % a.length]) < .03));
const bounds = p => [Math.min(...p.map(q => q[0])), Math.min(...p.map(q => q[1])),
                     Math.max(...p.map(q => q[0])), Math.max(...p.map(q => q[1]))];
const plan = CONFIG.pieces.map(p => ({ ...p, points: points(p.source) }));
plan.forEach(p => p.bounds = bounds(p.points));
const existing = await eda.pcb_PrimitiveFill.getAll();
const used = new Array(plan.length).fill(false);
for (const f of existing) {
  if (f.getState_Net() !== 'GND' || ![1, 2].includes(Number(f.getState_Layer()))) fail('Unexpected existing fill');
  const pts = points(f.getState_ComplexPolygon().getSource()), bb = bounds(pts);
  const index = plan.findIndex((p, i) => !used[i] && p.layer === Number(f.getState_Layer()) &&
    p.bounds.every((v, k) => Math.abs(v - bb[k]) < .05) && sameOutline(pts, p.points));
  if (index < 0) fail('Existing fill not in validated plan; do not stack copper');
  used[index] = true;
}
// All source/geometry checks above occur before any write.
const started = Date.now();
let created = 0, remaining = 0;
for (let i = 0; i < plan.length; i++) {
  if (used[i]) continue;
  if (Date.now() - started > 14500) { remaining++; continue; }
  const polygon = eda.pcb_MathPolygon.createPolygon(plan[i].source);
  if (!polygon) fail('Invalid prevalidated polygon');
  if (!await eda.pcb_PrimitiveFill.create(plan[i].layer, polygon, 'GND', 0, .2, false))
    fail('Fill creation failed; inspect before retry');
  created++;
}
if (!await eda.pcb_Document.save()) fail('Save failed');
return JSON.stringify({ kept: used.filter(Boolean).length, created, remaining,
  totalPlanned: plan.length, done: remaining === 0, saved: true });
