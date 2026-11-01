// Station board v0.3 PCB — step P5g (attendance template): silkscreen check inside EasyEDA (READ-ONLY), with EasyEDA's own text boxes.
// Every top-silk label and visible designator: overlap with any pad (real shape + 6 mil), any mounting hole, another
// text (labels + designators), or the board edge (15 mil).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\24_check_silk.js"
const CLR = 6, EDGE = 15;
const PROJECT = 'station_pcb';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);   // pcb_* calls fail while a schematic tab is active
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
// board extents from the layer-11 outline: a closed polyline since EasyEDA 3.2.206 (2026-10-25: pcb_PrimitiveLine is
// copper-only now); legacy layer-11 lines of older builds are read too
const _src = (p) => { try { return p.getState_Polygon().getSource() || []; } catch (e) { return []; } };
const _xs = [], _ys = [];
for (const p of ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((p) => String(p.getState_Layer()) === '11')) {
  _src(p).filter((v) => typeof v === 'number').forEach((v, i) => (i % 2 ? _ys : _xs).push(v));
}
for (const l of ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => String(l.getState_Layer()) === '11')) {
  _xs.push(l.getState_StartX(), l.getState_EndX()); _ys.push(l.getState_StartY(), l.getState_EndY());
}
const W = Math.max(..._xs);
const H = Math.max(..._ys);
const pads = [];
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const desOf = {};
for (const c of comps) {
  desOf[c.getState_PrimitiveId()] = c.getState_Designator();
  for (const p of (await c.getAllPins()) || []) {
    const sh = p.getState_Pad() || [];
    let w = +sh[1], h = +sh[2];
    if (!(w > 0 && h > 0)) w = h = 90;
    // normalise first: JS % keeps the sign (-90 % 180 = -90), so a pad at -90 deg was never swapped (robot J1.4)
    if (Math.abs((((p.getState_Rotation() % 180) + 180) % 180) - 90) < 1) [w, h] = [h, w];
    pads.push([p.getState_X() - w / 2, p.getState_Y() - h / 2, p.getState_X() + w / 2, p.getState_Y() + h / 2, c.getState_Designator() + '.' + p.getState_PadNumber()]);
  }
}
const holes = ((await eda.pcb_PrimitivePad.getAll()) || []).filter((p) => /^H[1-8]$/.test(String(p.getState_PadNumber())))
  .map((p) => { const sh = p.getState_Pad() || []; return [p.getState_X(), p.getState_Y(), (+sh[1] || 120) / 2, p.getState_PadNumber()]; });
const texts = [];
for (const s of ((await eda.pcb_PrimitiveString.getAll()) || []).filter((s) => String(s.getState_Layer()) === '3')) {
  const b = await eda.pcb_Primitive.getPrimitivesBBox([s.getState_PrimitiveId()]);
  if (b) texts.push(['"' + s.getState_Text() + '"', b]);
}
let hidden = 0;
for (const a of ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((a) => a.getState_Key() === 'Designator')) {
  if (!a.getState_ValueVisible()) { hidden++; continue; }
  const b = await eda.pcb_Primitive.getPrimitivesBBox([a.getState_PrimitiveId()]);
  if (b) texts.push([a.getState_Value(), b, desOf[a.getState_ParentPrimitiveId()]]);
}
const ov = (b, r, m) => Math.min(b.maxX, r[2]) - Math.max(b.minX, r[0]) > -m && Math.min(b.maxY, r[3]) - Math.max(b.minY, r[1]) > -m;
const onPad = [], onHole = [], onText = [], offBoard = [];
for (let i = 0; i < texts.length; i++) {
  const [name, b] = texts[i];
  const p = pads.find((r) => ov(b, r, CLR));
  if (p) onPad.push(name + '@' + p[4]);
  const h = holes.find(([x, y, r]) => Math.hypot(Math.max(b.minX - x, 0, x - b.maxX), Math.max(b.minY - y, 0, y - b.maxY)) < r + CLR);
  if (h) onHole.push(name + '@' + h[3]);
  if (b.minX < EDGE || b.minY < EDGE || b.maxX > W - EDGE || b.maxY > H - EDGE) offBoard.push(name);
  for (let j = 0; j < i; j++) {
    const q = texts[j][1];
    if (Math.min(b.maxX, q.maxX) - Math.max(b.minX, q.minX) > 1 && Math.min(b.maxY, q.maxY) - Math.max(b.minY, q.minY) > 1) onText.push(name + '~' + texts[j][0]);
  }
}
return JSON.stringify({ texts: texts.length, hiddenDesignators: hidden, onPad, onHole, onText, offBoard });
