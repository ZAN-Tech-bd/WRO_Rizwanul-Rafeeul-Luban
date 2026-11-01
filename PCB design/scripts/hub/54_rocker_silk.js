// Hub v0.4 — H-001 retry 2, step 4b (WRITES PCB1 silkscreen of WRO_HUB): free silk labels follow the parts moved by
// 53_rocker_local_moves.js (free texts don't move with their part). Each label shifts by its part's offset:
//   J4 -195 in y ("TO BUCK 12V", "+", "-"), J5 -125 in y ("-  5V FROM BUCK  +"), LED3 +450 in x ("5V").
// J3: the old VH-6 pin-1 mark "1" is deleted (the switch footprint carries its own "1"); "SWITCH J3" and the J3
// designator go into the gaps between the switch's pad rows (pads 9 mm along x, rows 413 mil apart in y).
// NOTE: 23_silkscreen.js still holds the OLD positions and deletes + redraws all free silk: do not re-run it before it
// is updated. Each text is checked at its "from" position first; abort on any mismatch. Idempotent.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\54_rocker_silk.js"
const PROJECT = 'WRO_HUB', PCB = 'ad35aed4d7610fd2';
const TEXTS = {   // id: [text, from, to, toRotation]
  '2622cb5db18f53ce': ['TO BUCK 12V', [210, 640], [210, 445], 90],
  '895f45fcdcf37d0d': ['+', [140, 782], [140, 587], 0],
  abdbc7846c7b0db8: ['-', [140, 498], [140, 303], 0],
  '49529d11268779e9': ['-  5V FROM BUCK  +', [504, 638], [504, 513], 90],
  e6e98663ecc3e9f5: ['5V', [330, 334], [780, 334], 0],
  '9adadbc418307667': ['SWITCH J3', [210, 1446], [-60, 1290], 0],
};
const DELETE = { '66c8cab3624981e5': ['1', [110, 1836]] };
const DES = ['0cbe9290d583d889e2', 'J3', [108, 1970], [-60, 1700], 0];
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || proj.friendlyName !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (!doc || doc.uuid !== PCB) return JSON.stringify({ abort: 'PCB1 not active', doc });
const at = (q, p) => Math.abs(q.getState_X() - p[0]) < 1 && Math.abs(q.getState_Y() - p[1]) < 1;
const strs = (await eda.pcb_PrimitiveString.getAll()) || [];
const res = { moved: [], ok: [], deleted: [], errors: [] };
const todo = [], del = [];
for (const [id, [text, from, to, rot]] of Object.entries(TEXTS)) {
  const s = strs.find((q) => q.getState_PrimitiveId() === id);
  if (!s || s.getState_Text() !== text) return JSON.stringify({ abort: 'text ' + id + ' "' + text + '" missing' });
  if (at(s, to) && s.getState_Rotation() === rot) { res.ok.push(text); continue; }
  if (!at(s, from)) return JSON.stringify({ abort: '"' + text + '" at ' + s.getState_X() + ',' + s.getState_Y() + ' (board changed)' });
  todo.push([id, text, to, rot]);
}
for (const [id, [text, from]] of Object.entries(DELETE)) {
  const s = strs.find((q) => q.getState_PrimitiveId() === id);
  if (!s) { res.ok.push('deleted ' + text); continue; }
  if (s.getState_Text() !== text || !at(s, from)) return JSON.stringify({ abort: 'delete target ' + id + ' changed' });
  del.push([id, text]);
}
const attrs = (await eda.pcb_PrimitiveAttribute.getAll()) || [];
const des = attrs.find((a) => a.getState_PrimitiveId() === DES[0]);
if (!des || des.getState_Value() !== DES[1]) return JSON.stringify({ abort: 'J3 designator attribute missing' });
const desTodo = !(at(des, DES[3]) && des.getState_Rotation() === DES[4]);
if (desTodo && !at(des, DES[2])) return JSON.stringify({ abort: 'J3 designator at ' + des.getState_X() + ',' + des.getState_Y() });
for (const [id, text, to, rot] of todo) {
  try { await eda.pcb_PrimitiveString.modify(id, { x: to[0], y: to[1], rotation: rot }); res.moved.push(text); }
  catch (e) { res.errors.push(text + ' ' + String(e).slice(0, 100)); }
}
if (del.length) { await eda.pcb_PrimitiveString.delete(del.map((d) => d[0])); res.deleted.push(...del.map((d) => d[1])); }
if (desTodo) {
  try { await eda.pcb_PrimitiveAttribute.modify(DES[0], { x: DES[3][0], y: DES[3][1], rotation: DES[4], alignMode: 5 }); res.moved.push('<J3>'); }
  catch (e) { res.errors.push('<J3> ' + String(e).slice(0, 100)); }
} else res.ok.push('<J3>');
await eda.pcb_Document.save();
return JSON.stringify(res);
