// Hub v0.3 PCB — designator check (read-only): visible designators touching a pad, or overlapping a top-silk label.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/26_check_designators.js"
const des = ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((a) => a.getState_Key() === 'Designator' && a.getState_ValueVisible());
const labels = [];
for (const s of ((await eda.pcb_PrimitiveString.getAll()) || []).filter((s) => String(s.getState_Layer()) === '3')) labels.push(await eda.pcb_Primitive.getPrimitivesBBox([s.getState_PrimitiveId()]));
const P = [];
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
for (const c of comps) for (const p of (await c.getAllPins()) || []) P.push([p.getState_X(), p.getState_Y(), /^J[1245]$/.test(c.getState_Designator()) ? 70 : 38]);
for (const p of (await eda.pcb_PrimitivePad.getAll()) || []) if (p.getState_PadNumber() === 'MH') P.push([p.getState_X(), p.getState_Y(), 118]);
const boxes = [];
const onPad = [], onLabel = [], onEachOther = [];
for (const a of des) {
  const b = await eda.pcb_Primitive.getPrimitivesBBox([a.getState_PrimitiveId()]);
  if (!b) continue;
  const v = a.getState_Value();
  if (P.some(([x, y, r]) => Math.hypot(Math.max(b.minX - x, 0, x - b.maxX), Math.max(b.minY - y, 0, y - b.maxY)) < r)) onPad.push(v);
  const ov = (q) => Math.min(b.maxX, q.maxX) - Math.max(b.minX, q.minX) > 2 && Math.min(b.maxY, q.maxY) - Math.max(b.minY, q.minY) > 2;
  if (labels.some(ov)) onLabel.push(v);
  for (const [w, q] of boxes) if (ov(q)) onEachOther.push(v + '~' + w);
  boxes.push([v, b]);
}
return JSON.stringify({ visible: des.length, onPad, onLabel, onEachOther });
