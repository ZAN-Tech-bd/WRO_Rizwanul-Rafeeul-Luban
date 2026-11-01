// Hub v0.3 — step 3c: dump every placed part's pins (number, name, x, y) for the net map (read-only).
// Output line per part: DES@x,y|num:name@x,y;...   Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/08_dump_pins.js"
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
const lines = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  const pins = (await c.getAllPins()) || [];
  const ps = pins.map((p) => `${p.getState_PinNumber()}:${p.getState_PinName()}@${Math.round(p.getState_X())},${Math.round(p.getState_Y())}`);
  lines.push(`${des}@${Math.round(c.getState_X())},${Math.round(c.getState_Y())}|${ps.join(';')}`);
}
return lines.sort().join('\n');
