// Hub v0.4b PCB — silkscreen check (read-only): every top-silkscreen text (free texts + designators) against
// (1) every pad / M3 hole with its REAL radius + 6 mil (v0.4b: XT pads are 5.5 / 6.0 / 3.5 mm now), (2) the board edge,
// (3) other silk texts. The fab clips silk on pads; overlapping texts are unreadable.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/24_check_silk.js"
// Board edge = the layer-11 outline's extents (H-001: left edge moved to x = -620; was a fixed 0..4600 box).
const outl = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((q) => String(q.getState_Layer()) === '11');
if (outl.length !== 1) return JSON.stringify({ abort: outl.length + ' outline polylines' });
const ov = outl[0].getState_Polygon().getSource().flat(Infinity).filter((n) => typeof n === 'number');
const X0 = Math.min(...ov.filter((_, i) => i % 2 === 0)), X1 = Math.max(...ov.filter((_, i) => i % 2 === 0));
const Y0 = Math.min(...ov.filter((_, i) => i % 2 === 1)), Y1 = Math.max(...ov.filter((_, i) => i % 2 === 1));
const radius = (p) => { const s = p.getState_Pad() || []; return Math.max(Number(s[1]) || 0, Number(s[2]) || 0) / 2 + 6; };
const strs = ((await eda.pcb_PrimitiveString.getAll()) || []).filter((s) => String(s.getState_Layer()) === '3');
const P = ((await eda.pcb_PrimitivePad.getAll()) || []).map((p) => [p.getState_X(), p.getState_Y(), radius(p)]);
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
for (const c of comps) for (const p of (await c.getAllPins()) || []) P.push([p.getState_X(), p.getState_Y(), radius(p)]);
const boxes = [];
for (const s of strs) boxes.push([s.getState_Text(), await eda.pcb_Primitive.getPrimitivesBBox([s.getState_PrimitiveId()])]);
const des = ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((a) => a.getState_Key() === 'Designator' && String(a.getState_Layer()) === '3');
for (const a of des) { try { boxes.push(['<' + a.getState_Value() + '>', await eda.pcb_Primitive.getPrimitivesBBox([a.getState_PrimitiveId()])]); } catch (e) { /* hidden */ } }
const hits = [], edge = [], overlap = [];
for (const [txt, b] of boxes) {
  if (!b) continue;
  for (const [x, y, r] of P) {
    const dx = Math.max(b.minX - x, 0, x - b.maxX), dy = Math.max(b.minY - y, 0, y - b.maxY);
    if (Math.hypot(dx, dy) < r) { hits.push(`${txt} @pad ${Math.round(x)},${Math.round(y)}`); break; }
  }
  if (b.minX < X0 + 10 || b.minY < Y0 + 10 || b.maxX > X1 - 10 || b.maxY > Y1 - 10) edge.push(txt);
}
for (let i = 0; i < boxes.length; i++) for (let j = i + 1; j < boxes.length; j++) {
  const a = boxes[i][1], b = boxes[j][1];
  if (a && b && Math.min(a.maxX, b.maxX) - Math.max(a.minX, b.minX) > 2 && Math.min(a.maxY, b.maxY) - Math.max(a.minY, b.minY) > 2) overlap.push(boxes[i][0] + ' / ' + boxes[j][0]);
}
return JSON.stringify({ texts: strs.length, designators: des.length, hits, edge, overlap });
