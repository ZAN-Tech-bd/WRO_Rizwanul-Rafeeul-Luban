// Hub v0.3 PCB — step 4e: put every footprint designator where it is readable and off the pads.
// R*/C*/D1: centred on the body between the two pads (rotated with the part); SB*: beside the header;
// station connectors: right of the "Sx Zy" label; LEDs: next to the LED; DevKit headers: at the USB end.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/25_designators.js"
const CENTER = 5, LEFT_MID = 2, RIGHT_MID = 8;
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
{ const p = await eda.dmt_Project.getCurrentProjectInfo(); if (!p || (p.friendlyName || p.name) !== 'WRO_HUB') return JSON.stringify({ abort: 'open project is not WRO_HUB' }); }
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const info = {};
for (const c of comps) {
  const pads = {};
  for (const p of (await c.getAllPins()) || []) pads[String(p.getState_PadNumber())] = [p.getState_X(), p.getState_Y()];
  info[c.getState_PrimitiveId()] = { des: c.getState_Designator(), x: c.getState_X(), y: c.getState_Y(), pads };
}
const target = (i) => {
  const d = i.des, p = i.pads;
  if (/^(R\d+|C\d+|D1)$/.test(d)) {
    const [a, b] = [p['1'], p['2']];
    const vertical = Math.abs(a[0] - b[0]) < 1;
    return [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, vertical ? 90 : 0, CENTER, 30];
  }
  // v0.4b: odd SBs sit on the right of their cell (next to the edge for S5 / S7): designator on their left
  if (/^SB[1357]$/.test(d)) return [Math.min(p['1'][0], p['2'][0]) - 80, p['1'][1], 0, RIGHT_MID, 30];
  if (/^SB\d$/.test(d)) return [Math.max(p['1'][0], p['2'][0]) + 80, p['1'][1], 0, LEFT_MID, 30];
  if (/^J1[0-7]$/.test(d)) return [(p['1'][0] + p['8'][0]) / 2 + 150, p['1'][1] + 175, 0, LEFT_MID, 36]; // clear of the M3 holes
  // v0.4b (2026-10-26 floor plan): LED1 / LED2 under their LED, LED3 upright on its right, DevKit at its USB end
  if (d === 'LED1' || d === 'LED2') return [i.x, i.y - 160, 0, CENTER, 28];
  if (d === 'LED3') return [i.x + 134, i.y, 90, CENTER, 24];
  if (d === 'J20') return [i.x + 210, i.y - 178, 0, CENTER, 24];   // right of its "E-STOP: ..." line (top edge above)
  if (d === 'J18') return [i.x + 210, i.y - 150, 0, CENTER, 24];   // right of its "I2C 3.3V: ..." line
  if (d === 'BZ1') return [i.x, i.y - 300, 0, CENTER, 28];          // in the gap above the servo ports
  if (/^J2[1-4]$/.test(d)) return [p['1'][0] - 138, p['1'][1], 90, CENTER, 24];   // left of pin 1 (5V)
  if (d === 'U1A') return [i.x + 140, 1700, 0, CENTER, 32];
  if (d === 'U1B') return [i.x - 140, 1700, 0, CENTER, 32];
  return null; // leave the rest where the footprint put them
};
const attrs = ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((a) => a.getState_Key() === 'Designator');
let moved = 0;
const errors = [];
for (const a of attrs) {
  const i = info[a.getState_ParentPrimitiveId()];
  const tgt = i && target(i);
  if (!tgt) continue;
  const [x, y, rot, align, size] = tgt;
  try {
    const m = a.toAsync();
    m.setState_X(x); m.setState_Y(y); m.setState_Rotation(rot); m.setState_AlignMode(align);
    m.setState_FontSize(size); m.setState_LineWidth(5);
    await m.done();
    moved++;
  } catch (e) { errors.push(i.des + ': ' + e); }
}
await eda.pcb_Document.save();
return JSON.stringify({ designators: attrs.length, moved, errors: errors.slice(0, 5) });
