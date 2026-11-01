// Robot carrier v0.5 — step P4i: add J15 (second obstacle port, XH-3, D-025) and put it into free space in block G
// (WRITES the schematic). 13_layout.js must NOT be re-run on the wired sheet (it re-packs every block and the API doesn't
// drag wires), so this script only moves J15: it measures every part's footprint on the sheet the way 13_layout.js does
// (symbol bbox ∪ pins ∪ a 20-unit stub along each pin's rotation, + 7 margin) and picks the free snapped spot closest to
// J11 inside block G's drawn frame. Block G was packed full in v0.4, so J15 then goes into the strip right below the frame
// (column 2 ends at y = 15) and the frame grows to include it; its title is updated. Then 14_wire.js wires J15's 3 pins.
// Idempotent: J15 is created only if missing; a J15 already in a free spot inside block G is left alone.
// Gates: WRO_Robot, schematic tab, window visible. Time-boxed.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\18_place_j15_v05.js"
const PROJECT = 'WRO_Robot';
const L = '0819f05c4eef4c71ace90d822a990e87';      // LCSC system library (10_resolve.js)
const XH3 = 'e3145416944046c8ae9e43248edd775d';    // B3B-XH-A C144394, the same device as J8 / J11 (11_place.js)
const G = ['J10', 'R14', 'R15', 'R16', 'R17', 'J11', 'R18', 'R19', 'C12', 'J12', 'SW1', 'R20', 'R21', 'C13', 'J13', 'SW2',
  'R22', 'R23', 'C14'];                             // block G as drawn in v0.4 (13_layout.js)
const TITLE = 16, STUB = 20, M = 7, SNAP = 5;
const DIR = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
const snap = (v) => Math.round(v / SNAP) * SNAP;
// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
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
let byDes = await comps();
if ((byDes.J15 || []).length > 1) return JSON.stringify({ abort: 'found ' + byDes.J15.length + ' J15 parts; nothing changed' });
let created = false;
if (!byDes.J15) {
  const comp = await eda.sch_PrimitiveComponent.create({ libraryUuid: L, uuid: XH3 }, 1300, 900); // staging, outside A4
  const a = comp.toAsync();
  a.setState_Designator('J15');
  await a.done();
  created = true;
  await sleep(1500);
  byDes = await comps();
  if (!byDes.J15) return JSON.stringify({ abort: 'J15 create did not commit yet: wait and re-run (never retry here)' });
}
// local rectangle (relative to the part origin): body + pins + stubs + margin (13_layout.js)
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
const others = [];
const rG = [];
for (const [d, list] of Object.entries(byDes)) {
  if (d === 'J15') continue;
  for (const c of list) {
    const r = await localRect(c);
    const abs = { d, x0: r.ox + r.x0, x1: r.ox + r.x1, y0: r.oy + r.y0, y1: r.oy + r.y1 };
    if (abs.x1 - abs.x0 > 600 || abs.y1 - abs.y0 > 600) continue;   // the A4 frame symbol, if it carries a designator
    others.push(abs);
    if (G.includes(d)) rG.push(abs);
  }
}
if (rG.length !== G.length) return JSON.stringify({ abort: 'block G parts missing', found: rG.map((r) => r.d) });
const j15 = byDes.J15[0];
const jr = await localRect(j15);
const j11 = byDes.J11[0];
// block G's drawn frame: the rectangle whose top-left sits 4 left / 3 above its "G · …" title (13_layout.js)
const title = ((await eda.sch_PrimitiveText.getAll()) || []).find((t) => /^G · /.test(t.getState_Content() || ''));
if (!title) return JSON.stringify({ abort: 'block G title not found' });
const frame = ((await eda.sch_PrimitiveRectangle.getAll()) || []).find((r) =>
  Math.abs(r.getState_TopLeftX() - (title.getState_X() - 4)) < 1 && Math.abs(r.getState_TopLeftY() - (title.getState_Y() + 3)) < 1);
if (!frame) return JSON.stringify({ abort: 'block G frame not found' });
const F = { x0: frame.getState_TopLeftX(), y1: frame.getState_TopLeftY(), w: frame.getState_Width(), h: Math.abs(frame.getState_Height()) };
const reg = { x0: F.x0 + 4, x1: F.x0 + F.w - 4, y0: F.y1 - F.h + 4, y1: F.y1 - TITLE };
const hit = (a) => others.some((b) => Math.min(a.x1, b.x1) - Math.max(a.x0, b.x0) > 0.01 && Math.min(a.y1, b.y1) - Math.max(a.y0, b.y0) > 0.01);
const at = (ox, oy) => ({ x0: ox + jr.x0, x1: ox + jr.x1, y0: oy + jr.y0, y1: oy + jr.y1 });
const inReg = (a, r) => a.x0 >= r.x0 && a.x1 <= r.x1 && a.y0 >= r.y0 && a.y1 <= r.y1;
const search = (r) => {
  let best = null;
  for (let oy = snap(r.y1 - jr.y1); oy >= r.y0 - jr.y0; oy -= SNAP) {
    for (let ox = snap(r.x0 - jr.x0); ox <= r.x1 - jr.x1; ox += SNAP) {
      const a = at(ox, oy);
      if (!inReg(a, r) || hit(a)) continue;
      const dist = Math.hypot(ox - j11.getState_X(), oy - j11.getState_Y());
      if (!best || dist < best.dist) best = { ox, oy, dist };
    }
  }
  return best;
};
const TITLE_V05 = 'G · I2C, 2 x obstacle sensor (J11 + J15, wired-OR), MUTE / spare buttons';
const cur = at(jr.ox, jr.oy);
if (!created && inReg(cur, reg) && !hit(cur)) {
  if (title.getState_Content() !== TITLE_V05) await eda.sch_PrimitiveText.modify(title.getState_PrimitiveId(), { content: TITLE_V05 });
  await eda.sch_Document.save();
  return JSON.stringify({ alreadyOk: true, J15: [Math.round(jr.ox), Math.round(jr.oy)], frame: F });
}
// 1) free spot inside the frame; 2) else in the strip right below it (column 2 ends at y = 15), and the frame grows
let best = search(reg), extended = false;
if (!best) {
  const below = { x0: reg.x0, x1: reg.x1, y0: Math.max(19, reg.y0 - (jr.y1 - jr.y0) - 10), y1: reg.y1 };
  best = search(below);
  extended = true;
}
if (!best) {
  return JSON.stringify({ abort: 'no free spot in or below block G', frame: F, j15Rect: jr });
}
const a = j15.toAsync();
a.setState_X(best.ox);
a.setState_Y(best.oy);
await a.done();
await sleep(1500);
const placed = at(best.ox, best.oy);
let frameHeight = null;
if (placed.y0 - 4 < F.y1 - F.h) {
  frameHeight = Math.round((F.y1 - (placed.y0 - 4)) * 10) / 10;
  await eda.sch_PrimitiveRectangle.modify(frame.getState_PrimitiveId(), { height: frameHeight });
}
if (title.getState_Content() !== TITLE_V05) await eda.sch_PrimitiveText.modify(title.getState_PrimitiveId(), { content: TITLE_V05 });
await sleep(1500);
await eda.sch_Document.save();
const back = (await comps()).J15[0];
const fr = ((await eda.sch_PrimitiveRectangle.getAll()) || []).find((r) => r.getState_PrimitiveId() === frame.getState_PrimitiveId());
return JSON.stringify({ created, moved: true, extended, J15: [Math.round(back.getState_X()), Math.round(back.getState_Y())],
  target: [best.ox, best.oy], J11: [Math.round(j11.getState_X()), Math.round(j11.getState_Y())], j15Rect: placed,
  frameBefore: F, frameAfter: fr && [fr.getState_TopLeftX(), fr.getState_TopLeftY(), fr.getState_Width(), fr.getState_Height()],
  rotation: back.getState_Rotation && back.getState_Rotation() });
