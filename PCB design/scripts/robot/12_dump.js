// Robot carrier v0.4 — step P4c: dump every placed part's bbox and pins (READ-ONLY).
// Line per part: DES@x,y[w x h@origin offset]|num:name@dx,dy,rot;...  (pin coords relative to the part origin)
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\12_dump.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\data\sch_dump_v04.txt"
const PROJECT = 'WRO_Robot';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
const lines = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  const x = Math.round(c.getState_X()), y = Math.round(c.getState_Y());
  let bb = null;
  try { bb = await eda.sch_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]); } catch (e) { bb = null; }
  const box = bb ? `${Math.round(bb.maxX - bb.minX)}x${Math.round(bb.maxY - bb.minY)}@${Math.round(bb.minX - x)},${Math.round(bb.minY - y)}` : '?';
  const pins = (await c.getAllPins()) || [];
  const ps = pins.map((p) => `${p.getState_PinNumber()}:${p.getState_PinName()}@${Math.round(p.getState_X()) - x},${Math.round(p.getState_Y()) - y},${Math.round(p.getState_Rotation())}`);
  lines.push(`${des}@${x},${y}[${box}]|${ps.join(';')}`);
}
return lines.sort().join('\n');
