// Hub v0.3 PCB — step 4c: verify the placement (read-only): footprint overlaps, parts off the board, hole
// clearance, DevKit header geometry (rows 1000 mil apart, pin 1 at the top, pin 1 <= 100 mil from the top edge).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/22_check_placement.js"
const W = 5200, H = 3900, HOLE_R = 118 + 15;
const OVERHANG_OK = new Set(['J1', 'J2', 'J4', 'J5']); // right-angle XT connectors stick out of the left edge
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids);
const box = {};
const pad = {};
for (const c of comps) {
  const d = c.getState_Designator();
  const bb = await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]);
  box[d] = bb;
  pad[d] = {};
  for (const p of (await c.getAllPins()) || []) pad[d][String(p.getState_PadNumber())] = [p.getState_X(), p.getState_Y()];
}
const des = Object.keys(box);
const overlaps = [];
for (let i = 0; i < des.length; i++) for (let j = i + 1; j < des.length; j++) {
  const a = box[des[i]], b = box[des[j]];
  const ox = Math.min(a.maxX, b.maxX) - Math.max(a.minX, b.minX), oy = Math.min(a.maxY, b.maxY) - Math.max(a.minY, b.minY);
  if (ox > 1 && oy > 1) overlaps.push(`${des[i]}~${des[j]} (${Math.round(ox)}x${Math.round(oy)})`);
}
const offBoard = des.filter((d) => {
  const b = box[d];
  return !OVERHANG_OK.has(d) && (b.minX < 0 || b.minY < 0 || b.maxX > W || b.maxY > H);
}).map((d) => `${d}[${Math.round(box[d].minX)},${Math.round(box[d].minY)}..${Math.round(box[d].maxX)},${Math.round(box[d].maxY)}]`);
const holes = [[170, 170], [W - 170, 170], [170, H - 170], [W - 170, H - 170]];
const nearHoles = des.filter((d) => holes.some(([hx, hy]) => {
  const b = box[d];
  const dx = Math.max(b.minX - hx, 0, hx - b.maxX), dy = Math.max(b.minY - hy, 0, hy - b.maxY);
  return Math.hypot(dx, dy) < HOLE_R;
}));
const u1a = pad.U1A || {}, u1b = pad.U1B || {};
const devkit = u1a['1'] && u1b['1'] ? {
  rowSpacing: Math.round(u1b['1'][0] - u1a['1'][0]),
  pin1Y: Math.round(u1a['1'][1]), pin22Y: Math.round(u1a['22'][1]),
  pin1FromTopEdge: Math.round(H - u1a['1'][1]),
  rowsAligned: Math.abs(u1a['1'][1] - u1b['1'][1]) < 1,
} : 'headers not found';
return JSON.stringify({ parts: des.length, overlaps, offBoard, nearHoles, devkit }, null, 1);
