// Hub v0.4 PCB — H-005 follow-up: two designators that collided after R7-R10 moved under the DevKit (WRITES: the
// Designator attributes of R6 and U1A only). R6's sat left of R6 on R10; U1A's sat on R7.1. R6 -> on its own body,
// vertical; U1A -> just below its pin 22. Idempotent; checked afterwards with 24_check_silk.js.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\63_servo_designators.js"
const PROJECT = 'WRO_HUB';
const WANT = { R6: [2145, 1805, 90], U1A: [1535, 1590, 0] };
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const desOf = {};
for (const c of (await eda.pcb_PrimitiveComponent.getAll()) || []) desOf[c.getState_PrimitiveId()] = c.getState_Designator();
const res = { moved: [], skipped: [] };
for (const a of ((await eda.pcb_PrimitiveAttribute.getAll()) || []).filter((x) => x.getState_Key() === 'Designator')) {
  const d = desOf[a.getState_ParentPrimitiveId()];
  const w = WANT[d];
  if (!w) continue;
  if (Math.abs(a.getState_X() - w[0]) < 0.5 && Math.abs(a.getState_Y() - w[1]) < 0.5 && a.getState_Rotation() === w[2]) { res.skipped.push(d); continue; }
  const m = a.toAsync();
  m.setState_X(w[0]); m.setState_Y(w[1]); m.setState_Rotation(w[2]); m.setState_AlignMode(5);
  await m.done();
  res.moved.push(d);
}
await eda.pcb_Document.save(tab.tabId);
return JSON.stringify(res);
