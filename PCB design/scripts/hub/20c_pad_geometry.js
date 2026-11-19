// Hub v0.4 PCB (copy of the robot script) — step P5d (from the attendance version): pad geometry + silkscreen text metrics for the offline silkscreen planner.
// READ-ONLY for the board, except one temporary text off the board (created, measured, deleted) to learn the font.
// Output: {pads: ["DES|pad|x|y|rot|shape|w|h|layer"], designators: ["DES|size|lineWidth|visible"], font: {...}}
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\20c_pad_geometry.js" > data\pad_geometry.json
const PROJECT = 'WRO_HUB';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
// pcb_* reads fail while a schematic tab is active: activate the PCB tab first
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const r1 = (v) => Math.round(v * 10) / 10;
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const pads = [];
for (const c of comps) {
  const d = c.getState_Designator();
  for (const p of (await c.getAllPins()) || []) {
    const sh = p.getState_Pad() || [];
    pads.push([d, p.getState_PadNumber(), r1(p.getState_X()), r1(p.getState_Y()), p.getState_Rotation(), sh[0], r1(sh[1] || 0), r1(sh[2] || 0), p.getState_Layer()].join('|'));
  }
}
const designators = [];
for (const a of ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((a) => a.getState_Key() === 'Designator')) {
  designators.push([a.getState_Value(), a.getState_FontSize(), a.getState_LineWidth(), a.getState_ValueVisible()].join('|'));
}
// font metrics: one temporary string far off the board
const font = {};
for (const [txt, size] of [['R10', 32], ['LED5', 32], ['W', 100], ['1', 100]]) {
  const s = await eda.pcb_PrimitiveString.create(3, -5000, -5000, txt, 'default', size, 6, 5, 0, false, 0, false, false);
  { const _t = Date.now(); while (Date.now() - _t < 300) await eda.dmt_Project.getCurrentProjectInfo(); } // no timers (hidden window)
  const b = await eda.pcb_Primitive.getPrimitivesBBox([s.getState_PrimitiveId()]);
  font[txt + '@' + size] = { w: r1(b.maxX - b.minX), h: r1(b.maxY - b.minY), cx: r1((b.maxX + b.minX) / 2 + 5000), cy: r1((b.maxY + b.minY) / 2 + 5000) };
  await eda.pcb_PrimitiveString.delete([s.getState_PrimitiveId()]);
}
return JSON.stringify({ pads, designators, font });
