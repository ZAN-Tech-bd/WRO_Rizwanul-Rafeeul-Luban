// WRITE Robot PCB1 ONLY: relocate the flagged silk labels (STRING x/y) and the SW1
// designator ATTR to collision-checked spots planned offline from the saved export
// (plan_silk_fix.py). Guards: exact before positions; fresh readback; no other edits.
const PLAN = {"strings":[{"id":"dd5061e70edf6d25","text":"470uF","before":[1300.8,1260.1],"after":[1320.8,1260.1]},{"id":"53151346fa2ab82c","text":"470uF","before":[2900.9,514.9],"after":[2886.8,529.0]},{"id":"95943be94a930ea9","text":"+","before":[3165,1502.5],"after":[3185.0,1502.5]}],"designators":[{"id":"2e78bde06cf0b009e26","des":"SW1","before":[1199.7,269.2],"after":[1182.3,170.8]}]};
const PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864';
const PCB = '643ce99542452c8f';
const p = await eda.dmt_Project.getCurrentProjectInfo();
if (p?.uuid !== PROJECT || p?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.documentType !== 3 || d?.uuid !== PCB) throw new Error('Open Robot PCB1');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const near = (a, b) => Math.abs(a - b) < 0.5;
const results = [];
for (const e of PLAN.strings) {
  const got = await eda.pcb_PrimitiveString.get([e.id]);
  const s = Array.isArray(got) ? got[0] : got;
  if (!s || s.getState_Text() !== e.text || !near(s.getState_X(), e.before[0]) || !near(s.getState_Y(), e.before[1]))
    throw new Error('Source guard failed for string ' + e.id);
  if (!await eda.pcb_PrimitiveString.modify(e.id, { x: e.after[0], y: e.after[1] }))
    throw new Error('String move failed ' + e.id);
  const back = await eda.pcb_PrimitiveString.get([e.id]);
  const b = Array.isArray(back) ? back[0] : back;
  if (!near(b.getState_X(), e.after[0]) || !near(b.getState_Y(), e.after[1]))
    throw new Error('String readback failed ' + e.id);
  results.push({ text: e.text, from: e.before, to: e.after });
}
for (const e of PLAN.designators) {
  const attrs = ((await eda.pcb_PrimitiveAttribute.getAll()) || [])
    .filter(a => a.getState_PrimitiveId() === e.id && a.getState_Key() === 'Designator');
  if (attrs.length !== 1 || !near(attrs[0].getState_X(), e.before[0]) || !near(attrs[0].getState_Y(), e.before[1]))
    throw new Error('Designator guard failed for ' + e.des);
  const m = attrs[0].toAsync();
  m.setState_X(e.after[0]);
  m.setState_Y(e.after[1]);
  await m.done();
  const check = ((await eda.pcb_PrimitiveAttribute.getAll()) || []).find(a => a.getState_PrimitiveId() === e.id);
  if (!near(check.getState_X(), e.after[0]) || !near(check.getState_Y(), e.after[1]))
    throw new Error('Designator readback failed for ' + e.des);
  results.push({ text: e.des + ' (designator)', from: e.before, to: e.after });
}
if (!await eda.pcb_Document.save()) throw new Error('Save failed');
return JSON.stringify({ moved: results, saved: true,
  verification: 're-run 24_check_silk; expect onPad/onText/offBoard clear for these items' });
