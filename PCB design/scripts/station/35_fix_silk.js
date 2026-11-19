// WRITE station_pcb PCB ONLY: (1) move the J1 designator out of the HUB CABLE label
// (review finding 1), (2) update the board identifier string V0.3 -> V0.4 (review finding 2).
// Guards: exact source values; fresh readback; no other edits. Export diff required after.
const PROJECT = '2d2da5292f828ea674cc52b5833bd8a226b03f5595240d96a18801ebe8c8acea';
const PCB = 'fdcb20516f125209';
const p = await eda.dmt_Project.getCurrentProjectInfo();
if (p?.uuid !== PROJECT) throw new Error('Wrong project (need station_pcb)');
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.documentType !== 3 || d?.uuid !== PCB) throw new Error('Open the station PCB tab');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const near = (a, b) => Math.abs(a - b) < 0.5;
const out = {};
// 1) J1 designator ATTR
const attrs = ((await eda.pcb_PrimitiveAttribute.getAll()) || [])
  .filter(a => a.getState_PrimitiveId() === '3a54f12f9aef84efe42' && a.getState_Key() === 'Designator');
if (attrs.length !== 1 || attrs[0].getState_Value() !== 'J1' ||
    !near(attrs[0].getState_X(), 1125) || !near(attrs[0].getState_Y(), 180))
  throw new Error('J1 designator guard failed: ' + JSON.stringify(
    attrs[0] && { v: attrs[0].getState_Value(), x: attrs[0].getState_X(), y: attrs[0].getState_Y() }));
const dm = attrs[0].toAsync();
dm.setState_X(1500);
dm.setState_Y(125);
await dm.done();
const dchk = ((await eda.pcb_PrimitiveAttribute.getAll()) || []).find(a => a.getState_PrimitiveId() === '3a54f12f9aef84efe42');
if (dchk.getState_Value() !== 'J1' || !near(dchk.getState_X(), 1500) || !near(dchk.getState_Y(), 125))
  throw new Error('J1 designator readback failed');
out.designator = { id: '3a54f12f9aef84efe42', from: [1125, 180], to: [1500, 125] };
// 2) identifier string V0.3 -> V0.4 (position/style untouched)
const got = await eda.pcb_PrimitiveString.get(['1f7c210019de45fd']);
const s = Array.isArray(got) ? got[0] : got;
const before = s.getState_Text();
if (!near(s.getState_X(), 1546.8) || !near(s.getState_Y(), 1064.8) || !/V0\.3/.test(before))
  throw new Error('Identifier guard failed: ' + JSON.stringify({ text: before, x: s.getState_X(), y: s.getState_Y() }));
const after = before.replace('V0.3', 'V0.4');
if (!await eda.pcb_PrimitiveString.modify('1f7c210019de45fd', { text: after }))
  throw new Error('Identifier modify failed');
const s2 = await eda.pcb_PrimitiveString.get(['1f7c210019de45fd']);
const s2v = (Array.isArray(s2) ? s2[0] : s2).getState_Text();
if (!/V0\.4/.test(s2v) || /V0\.3/.test(s2v)) throw new Error('Identifier readback failed: ' + s2v);
out.identifier = { id: '1f7c210019de45fd', from: before, to: after };
if (!await eda.pcb_Document.save()) throw new Error('Save failed');
return JSON.stringify({ ...out, saved: true,
  verification: 'fresh full export + exact-change diff; then strict DRC + Gerber audit' });
