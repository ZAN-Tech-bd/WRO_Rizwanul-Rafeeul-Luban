// Hub v0.3 — fix ERC fatal "endpoints of pins overlap and are not connected": J4.1 and J5.4 (both XT mounting
// tabs) landed on the same point when the sheet was re-arranged. Nudge J5 to the first offset where its pins and
// redrawn stubs touch nothing else, move it, and replace its two stubs (+5V, GND). The API does not drag wires.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/13_fix_overlap.js"
const MOVE = 'J5';
const NETS = { 1: 'GND', 2: '+5V' }; // XT30 male: 1 = -, 2 = +
const L = 30;
const DIR = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const key = (x, y) => Math.round(x) + ',' + Math.round(y);
const onSeg = (p, a, b) => {
  const cross = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
  return Math.abs(cross) < 0.5 && p[0] >= Math.min(a[0], b[0]) && p[0] <= Math.max(a[0], b[0]) &&
    p[1] >= Math.min(a[1], b[1]) && p[1] <= Math.max(a[1], b[1]);
};

const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
let target = null;
const otherPins = [];
for (const c of comps) {
  const d = c.getState_Designator && c.getState_Designator();
  if (!d) continue;
  const pins = (await c.getAllPins()) || [];
  if (d === MOVE) target = { c, pins };
  else for (const p of pins) otherPins.push([Math.round(p.getState_X()), Math.round(p.getState_Y())]);
}
const myPinKeys = new Set(target.pins.map((p) => key(p.getState_X(), p.getState_Y())));
const allWires = ((await eda.sch_PrimitiveWire.getAll()) || []).map((w) => {
  const f = (w.getState_Line() || []).flat(3).map(Math.round);
  const pts = [];
  for (let i = 0; i + 1 < f.length; i += 2) pts.push([f[i], f[i + 1]]);
  return { id: w.getState_PrimitiveId(), pts };
});
const mine = allWires.filter((w) => w.pts.some((p) => myPinKeys.has(key(p[0], p[1]))));
const others = allWires.filter((w) => !mine.includes(w));
const segsOf = (w) => w.pts.slice(1).map((p, i) => [w.pts[i], p]);
const otherSegs = others.flatMap(segsOf);

const plan = (dx, dy) => {
  const pins = target.pins.map((p) => ({
    n: String(p.getState_PinNumber()), x: Math.round(p.getState_X()) + dx, y: Math.round(p.getState_Y()) + dy,
    rot: ((Math.round(p.getState_Rotation()) % 360) + 360) % 360,
  }));
  const stubs = pins.filter((p) => NETS[p.n]).map((p) => {
    const [ux, uy] = DIR[p.rot] || [-1, 0];
    return { net: NETS[p.n], a: [p.x, p.y], b: [p.x + ux * L, p.y + uy * L] };
  });
  for (const p of pins) {
    if (otherPins.some((q) => q[0] === p.x && q[1] === p.y)) return null;
    if (otherSegs.some(([a, b]) => onSeg([p.x, p.y], a, b))) return null;
  }
  for (const s of stubs) {
    if (otherPins.some((q) => onSeg(q, s.a, s.b))) return null;
    if (otherSegs.some(([a, b]) => onSeg(s.a, a, b) || onSeg(s.b, a, b) || onSeg(a, s.a, s.b) || onSeg(b, s.a, s.b))) return null;
  }
  return { dx, dy, stubs };
};
const tries = [[0, 20], [0, -20], [0, 30], [0, -30], [10, 0], [-10, 0], [0, 40], [0, -40], [20, 0], [-20, 0]];
let chosen = null;
for (const [dx, dy] of tries) { chosen = plan(dx, dy); if (chosen) break; }
if (!chosen) return JSON.stringify({ ok: false, reason: 'no clear offset found' });

if (mine.length) await eda.sch_PrimitiveWire.delete(mine.map((w) => w.id));
const a = target.c.toAsync();
a.setState_X(target.c.getState_X() + chosen.dx);
a.setState_Y(target.c.getState_Y() + chosen.dy);
await a.done();
await sleep(1500);
for (const s of chosen.stubs) await eda.sch_PrimitiveWire.create([s.a[0], s.a[1], s.b[0], s.b[1]], s.net);
await sleep(2000);
// re-check: any pins of different parts still on one point?
const comps2 = await eda.sch_PrimitiveComponent.get(await eda.sch_PrimitiveComponent.getAllPrimitiveId());
const at = {};
for (const c of comps2) {
  const d = c.getState_Designator && c.getState_Designator();
  if (!d) continue;
  for (const p of (await c.getAllPins()) || []) (at[key(p.getState_X(), p.getState_Y())] = at[key(p.getState_X(), p.getState_Y())] || []).push(d + '.' + p.getState_PinNumber());
}
const overlaps = Object.entries(at).filter(([, v]) => new Set(v.map((s) => s.split('.')[0])).size > 1);
await eda.sch_Document.save();
return JSON.stringify({ ok: true, moved: MOVE, by: [chosen.dx, chosen.dy], oldStubsDeleted: mine.length, stubs: chosen.stubs, overlaps });
