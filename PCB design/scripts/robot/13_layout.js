// Robot carrier v0.5 — step P4d: pack the staged parts into labelled function blocks inside the A4 frame (WRITES).
// Each part's footprint on the sheet = symbol bbox + every pin end + a STUB-long wire in the pin direction (the
// stub 14_wire.js will draw) + margin, so no stub can reach another part's pin. Columns of blocks, shelf rows inside a
// block. Idempotent: parts already at their target are skipped; block frames/titles are drawn once.
// Sheet: A4 = 1170 x 825 (10 mil), origin bottom-left, Y up; title block bottom-right (x >= 815, y <= 175) is reserved.
// Column split / block order: choose with sch_pack.py (offline, from 12_dump) before running this.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\13_layout.js"
const PROJECT = 'WRO_Robot';
const STUB = 20, M = 7, GAP = 10, TITLE = 16, SNAP = 5;
// sch_pack.py 2026-10-16 (data/sch_dump_v04.txt): split 320/735, heights 584 / 580 / 403, slack 211 units.
// v0.5 (2026-10-25): NEVER re-run this on the wired v0.4 sheet — it re-packs every block from these lists and the API
// doesn't drag wires. The lists below are v0.5 (for a fresh sheet only); the live sheet keeps its v0.4 frames, and
// 18_place_j15_v05.js put J15 into block G.
const COLS = [
  { x0: 25, x1: 345, y1: 800, y0: 15, blocks: ['B', 'C', 'D'] },
  { x0: 355, x1: 735, y1: 800, y0: 15, blocks: ['E', 'G'] },
  { x0: 745, x1: 1145, y1: 800, y0: 185, blocks: ['A', 'F'] }, // above the title block (x >= 815, y <= 175)
];
const BLOCKS = {
  A: ['Power entry, switch, buck feeds, 5 V in, LEDs, battery sense', ['J1', 'J5', 'J2', 'J3', 'J4', 'R26', 'LED1',
    'R27', 'LED2', 'R24', 'R25', 'C11']],
  B: ['YD-ESP32-S3 N16R8 sockets (pin 1 = antenna end) + EXP', ['U1A', 'U1B', 'J14']],
  C: ['L298N motor port', ['J6', 'R2', 'R3']],
  D: ['RC522 RFID port (cable)', ['J7', 'R4', 'R5', 'R6', 'C7', 'C8']],
  E: ['DFPlayer + 2 x LM386 sockets + VAMP rail / JP1', ['U2A', 'U2B', 'C3', 'C4', 'R7', 'R8', 'R9', 'U3', 'U4', 'D1', 'D2',
    'JP1', 'C9', 'C10']],
  F: ['ESP32-CAM + Mega display ports', ['J8', 'C5', 'C6', 'R10', 'J9', 'R11', 'R12', 'R13']],
  G: ['I2C, obstacle sensor, MUTE / spare buttons', ['J10', 'R14', 'R15', 'R16', 'R17', 'J11', 'J15', 'R18', 'R19', 'C12', 'J12', 'SW1',
    'R20', 'R21', 'C13', 'J13', 'SW2', 'R22', 'R23', 'C14']],
};
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
await sleep(300);
const t0 = Date.now();

const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
const byDes = {};
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const d = c.getState_Designator && c.getState_Designator();
  if (d) byDes[d] = c;
}
// local rectangle (relative to the part origin) covering body, pins and their future stubs
const rect = {};
for (const [d, c] of Object.entries(byDes)) {
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
  rect[d] = { x0: x0 - M, x1: x1 + M, y0: y0 - M, y1: y1 + M };
}
// pack
const target = {}, frames = [], overflow = [];
for (const col of COLS) {
  let top = col.y1;
  for (const b of col.blocks) {
    const [title, list] = BLOCKS[b];
    const bx0 = col.x0, bx1 = col.x1, btop = top;
    let cx = bx0 + 4, cy = btop - TITLE, rowH = 0;
    for (const d of list) {
      const r = rect[d];
      if (!r) { overflow.push(d + ':missing'); continue; }
      const w = r.x1 - r.x0, h = r.y1 - r.y0;
      if (cx + w > bx1 - 4 && cx > bx0 + 4) { cy -= rowH; cx = bx0 + 4; rowH = 0; }
      // part origin so that its rect's top-left sits at (cx, cy)
      target[d] = [snap(cx - r.x0), snap(cy - r.y1)];
      cx += w; rowH = Math.max(rowH, h);
    }
    const bottom = cy - rowH - 4;
    frames.push({ b, title, x0: bx0, x1: bx1, y1: btop, y0: bottom });
    if (bottom < col.y0) overflow.push(b + ' overflows by ' + Math.round(col.y0 - bottom));
    top = bottom - GAP;
  }
}
if (overflow.length) return JSON.stringify({ ok: false, overflow, frames: frames.map((f) => [f.b, f.y1, Math.round(f.y0)]) });
// move parts
let moved = 0, already = 0;
const errors = [];
for (const [d, [x, y]] of Object.entries(target)) {
  const c = byDes[d];
  if (Math.round(c.getState_X()) === x && Math.round(c.getState_Y()) === y) { already++; continue; }
  if (Date.now() - t0 > 18000) break;
  try { const a = c.toAsync(); a.setState_X(x); a.setState_Y(y); await a.done(); moved++; } catch (e) { errors.push(d + ' ' + e); }
}
const remaining = Object.keys(target).length - moved - already;
// frames + titles once, after every part is in place
let drawn = 0;
if (!remaining) {
  const texts = ((await eda.sch_PrimitiveText.getAll()) || []).map((t) => t.getState_Content && t.getState_Content());
  for (const f of frames) {
    const label = f.b + ' · ' + f.title;
    if (texts.includes(label)) continue;
    try {
      await eda.sch_PrimitiveRectangle.create(f.x0, f.y1, f.x1 - f.x0, f.y1 - f.y0, 0, 0, '#6a7f99', null, 1, 1, 'None');
      await eda.sch_PrimitiveText.create(f.x0 + 4, f.y1 - 3, label, 0, '#1f4e8c', null, 8, true, false, false, 1);
      drawn++;
    } catch (e) { errors.push('frame ' + f.b + ' ' + e); }
  }
  await sleep(1500);
  await eda.sch_Document.save();
}
return JSON.stringify({ ok: true, moved, already, remaining, framesDrawn: drawn,
  frames: frames.map((f) => [f.b, f.x0, f.x1, f.y1, Math.round(f.y0)]), errors: errors.slice(0, 8) });
