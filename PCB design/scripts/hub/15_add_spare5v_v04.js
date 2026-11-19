// Hub v0.4 — step 3i: add the spare 5 V ports J25 / J26 (JST XH 2-pin, user 2026-10-25) to the live sheet (WRITES).
// The hub sheet was arranged by hand (v0.3 review), so 07_place.js coordinates don't apply: this script measures every
// part's footprint (symbol bbox ∪ pins ∪ a 30-unit stub along each pin, + 7 margin) and puts each new port at the free
// snapped spot closest to J5 (5 V from the buck) inside the A4 frame, outside the title block. Then 09_wire_nets.js
// wires them (1 = +5V, 2 = GND). Idempotent: an existing, non-overlapping J25 / J26 is left alone.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\15_add_spare5v_v04.js"

const PROJECT = 'WRO_HUB';
const L = '0819f05c4eef4c71ace90d822a990e87';
const XH2 = '3b9af90046a04769ada1b68e95fdf6de';   // B2B-XH-A C378939 (robot 11_place.js)
const NEW = ['J25', 'J26'];
const STUB = 30, M = 7, SNAP = 5;
const FRAME = { x0: 15, y0: 15, x1: 1155, y1: 810 }, TB = { x0: 815, y1: 175 };   // A4 inner area, title block bottom-right
const DIR = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
const snap = (v) => Math.round(v / SNAP) * SNAP;
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(500);
const comps = async () => {
  const out = {};
  for (const c of (await eda.sch_PrimitiveComponent.get(await eda.sch_PrimitiveComponent.getAllPrimitiveId())) || []) {
    const d = c && c.getState_Designator && c.getState_Designator();
    if (d) (out[d] = out[d] || []).push(c);
  }
  return out;
};
const localRect = async (c) => {
  const ox = c.getState_X(), oy = c.getState_Y();
  const bb = await eda.sch_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]);
  let x0 = bb.minX - ox, x1 = bb.maxX - ox, y0 = bb.minY - oy, y1 = bb.maxY - oy;
  for (const p of (await c.getAllPins()) || []) {
    const px = p.getState_X() - ox, py = p.getState_Y() - oy;
    const [ux, uy] = DIR[((Math.round(p.getState_Rotation()) % 360) + 360) % 360] || [-1, 0];
    for (const [qx, qy] of [[px, py], [px + ux * STUB, py + uy * STUB]]) {
      x0 = Math.min(x0, qx); x1 = Math.max(x1, qx); y0 = Math.min(y0, qy); y1 = Math.max(y1, qy);
    }
  }
  return { x0: x0 - M, x1: x1 + M, y0: y0 - M, y1: y1 + M, ox, oy };
};
let byDes = await comps();
const created = [];
for (const [i, d] of NEW.entries()) {
  if ((byDes[d] || []).length > 1) return JSON.stringify({ abort: 'found several ' + d });
  if (!byDes[d]) {
    const c = await eda.sch_PrimitiveComponent.create({ libraryUuid: L, uuid: XH2 }, 1400 + i * 150, 900);   // staging, off the frame
    const a = c.toAsync(); a.setState_Designator(d); await a.done();
    created.push(d);
  }
}
if (created.length) { await sleep(1500); byDes = await comps(); }
if (NEW.some((d) => !byDes[d])) return JSON.stringify({ abort: 'create did not commit yet: wait and re-run', created });
const hit = (a, list) => list.some((b) => Math.min(a.x1, b.x1) - Math.max(a.x0, b.x0) > 0.01 && Math.min(a.y1, b.y1) - Math.max(a.y0, b.y0) > 0.01);
const inside = (a) => a.x0 >= FRAME.x0 && a.x1 <= FRAME.x1 && a.y0 >= FRAME.y0 && a.y1 <= FRAME.y1 && !(a.x1 > TB.x0 && a.y0 < TB.y1);
const others = [];
for (const [d, list] of Object.entries(byDes)) {
  if (NEW.includes(d)) continue;
  for (const c of list) {
    const r = await localRect(c);
    const abs = { d, x0: r.ox + r.x0, x1: r.ox + r.x1, y0: r.oy + r.y0, y1: r.oy + r.y1 };
    if (abs.x1 - abs.x0 > 600 || abs.y1 - abs.y0 > 600) continue;   // the A4 frame symbol
    others.push(abs);
  }
}
const j5 = byDes.J5[0];
const res = { created, placed: [] };
for (const d of NEW) {
  const c = byDes[d][0];
  const r = await localRect(c);
  const at = (ox, oy) => ({ d, x0: ox + r.x0, x1: ox + r.x1, y0: oy + r.y0, y1: oy + r.y1 });
  const cur = at(r.ox, r.oy);
  if (inside(cur) && !hit(cur, others)) { others.push(cur); res.placed.push([d, 'alreadyOk', Math.round(r.ox), Math.round(r.oy)]); continue; }
  let best = null;
  for (let oy = snap(FRAME.y1 - r.y1); oy >= FRAME.y0 - r.y0; oy -= SNAP) {
    for (let ox = snap(FRAME.x0 - r.x0); ox <= FRAME.x1 - r.x1; ox += SNAP) {
      const a = at(ox, oy);
      if (!inside(a) || hit(a, others)) continue;
      const dist = Math.hypot(ox - j5.getState_X(), oy - j5.getState_Y());
      if (!best || dist < best.dist) best = { ox, oy, dist };
    }
  }
  if (!best) { res.placed.push([d, 'NO FREE SPOT']); continue; }
  const a = c.toAsync(); a.setState_X(best.ox); a.setState_Y(best.oy); await a.done();
  others.push(at(best.ox, best.oy));
  res.placed.push([d, 'moved', best.ox, best.oy, Math.round(best.dist)]);
}
await sleep(1500);
await eda.sch_Document.save();
return JSON.stringify(res);
