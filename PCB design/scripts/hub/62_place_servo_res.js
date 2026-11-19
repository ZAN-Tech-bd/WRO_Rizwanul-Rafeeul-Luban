// Hub v0.4 PCB — H-005: place the five servo resistors that were still off-board (WRITES: position / rotation of R5, R7,
// R8, R9, R10 only; nothing else). Spots from a free-space search on the live board (part outlines + 30 mil, the user's
// tracks + 45 mil, holes); v1 (strip between J3 and U1A) sat on the user's I2C tracks and was replaced the same day.
//   R7-R10 (220R, SERVO_BUF -> J21-J24 SIG): four vertical resistors under the DevKit between U1A's pin row and R6
//     (x 1660 / 1780 / 1900 / 2020, >= 55 mil copper gap to U1A and R6); rot 90 puts pad 1 (SERVO_BUF) at the bottom,
//     toward U2.6.
//   R5 (10k, SERVO_IN pull-down): horizontal below U2 / C1, the nearest free spot to U2.5.
// Pads are read back and must sit where expected (+/- 1.5 mil). Idempotent: parts already there are skipped.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\62_place_servo_res.js"
const PROJECT = 'WRO_HUB';
const P = { R7: [1660, 1840, 90], R8: [1780, 1840, 90], R9: [1900, 1840, 90], R10: [2020, 1840, 90], R5: [1890, 932.5, 0] };
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const near = (a, b) => Math.abs(a - b) < 1.5;
const norm = (r) => ((Math.round(r) % 360) + 360) % 360;
// expected pad positions: pad 1 at local (-202.8, 0), pad 2 at (+202.8, 0) for these resistors, rotated CCW
const expect = (x, y, r) => { const a = r * Math.PI / 180, c = Math.cos(a), s = Math.sin(a);
  return { '1': [x - 202.8 * c, y - 202.8 * s], '2': [x + 202.8 * c, y + 202.8 * s] }; };
const res = { moved: [], ok: [], bad: [], missing: [] };
const comps = (await eda.pcb_PrimitiveComponent.getAll()) || [];
for (const [des, [x, y, r]] of Object.entries(P)) {
  const c = comps.find((k) => k.getState_Designator() === des);
  if (!c) { res.missing.push(des); continue; }
  if (near(c.getState_X(), x) && near(c.getState_Y(), y) && norm(c.getState_Rotation()) === r) { res.ok.push(des); continue; }
  const a = c.toAsync(); a.setState_X(x); a.setState_Y(y); a.setState_Rotation(r); await a.done();
  res.moved.push(des);
}
for (const des of Object.keys(P)) {
  const c = ((await eda.pcb_PrimitiveComponent.getAll()) || []).find((k) => k.getState_Designator() === des);
  if (!c) continue;
  const e = expect(...P[des]);
  for (const p of (await c.getAllPins()) || []) {
    const q = e[String(p.getState_PadNumber())];
    if (!q || !near(p.getState_X(), q[0]) || !near(p.getState_Y(), q[1]))
      res.bad.push(des + '.' + p.getState_PadNumber() + ' at ' + p.getState_X().toFixed(1) + ',' + p.getState_Y().toFixed(1) + ' net ' + p.getState_Net());
  }
}
await eda.pcb_Document.save(tab.tabId);
return JSON.stringify(res);
