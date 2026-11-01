// Hub v0.4 PCB — robot D-031 (2026-10-26): vertical XT drill holes set to the user's sizes (WRITES: hole and shape of the
// 8 component pads of J1 / J2 / J4 / J5; nets and positions untouched). Copy of robot/44_xt_pads.js.
//   J1 / J2 XT60UPB-M: hole 4.6 -> 4.0 mm, pad 5.5 mm kept (ring 0.75 mm); J4 XT60UPB-F: hole 4.7 -> 4.0 mm, pad 6.0 mm kept
//   J5 XT30UPB-M: hole 1.8 -> 2.5 mm, pad 2.5 -> 3.5 mm (ring 0.5 mm, 1.5 mm gap at 5.0 mm pitch)
// API BUG (pro-api 0.2.53): a component pad's getState_Hole() reports 10-mil units, toAsync()...done() writes mil, so the
// hole is ALWAYS set explicitly in mil. Import Changes resets these pads to the library sizes: RE-RUN after every import.
// Checks every pad first (supplier id, 2 round pads) and aborts unchanged otherwise. Idempotent.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\44_xt_pads.js"
const PROJECT = 'WRO_HUB';
const MM = 1 / 0.0254;
const XT60 = { hole: 4.0 * MM, pad: 5.5 * MM }, XT60F = { hole: 4.0 * MM, pad: 6.0 * MM };
const XT30 = { hole: 2.5 * MM, pad: 3.5 * MM };
const PARTS = { J1: ['C2918078', XT60], J2: ['C2918078', XT60], J4: ['C9900007500', XT60F], J5: ['C428721', XT30] };
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const near = (a, b, tol) => Math.abs(a - b) < tol;
const comps = async () => ((await eda.pcb_PrimitiveComponent.getAll()) || []).filter((c) => PARTS[c.getState_Designator()]);
const todo = [];
for (const c of await comps()) {
  const des = c.getState_Designator(), [lcsc, want] = PARTS[des];
  if (c.getState_SupplierId() !== lcsc) return JSON.stringify({ abort: des + ' is ' + c.getState_SupplierId() + ', expected ' + lcsc + ' (Import Changes first)' });
  const pins = await c.getAllPins();
  if (pins.length !== 2) return JSON.stringify({ abort: des + ' has ' + pins.length + ' pads, expected 2' });
  for (const p of pins) {
    const n = String(p.getState_PadNumber()), s = p.getState_Pad(), h = p.getState_Hole();
    if (!h || h[0] !== 'ROUND' || !s || s[0] !== 'ELLIPSE') return JSON.stringify({ abort: des + '.' + n + ' pad ' + JSON.stringify(s) + ' hole ' + JSON.stringify(h) });
    if (near(s[1], want.pad, 0.2) && near(s[2], want.pad, 0.2) && near(h[1], want.hole / 10, 0.02)) continue;   // getter: 10-mil units
    todo.push([p, des + '.' + n, want]);
  }
}
const found = (await comps()).map((c) => c.getState_Designator());
if (found.length !== 4) return JSON.stringify({ abort: 'found ' + found.join(',') + ', expected J1 J2 J4 J5' });
const res = { changed: [], skipped: 8 - todo.length, errors: [] };
for (const [p, name, want] of todo) {
  try {
    const a = p.toAsync();
    a.setState_Pad(['ELLIPSE', want.pad, want.pad]);
    a.setState_Hole(['ROUND', want.hole]);
    await a.done();
    res.changed.push(name);
  } catch (e) { res.errors.push(name + ' ' + String(e).slice(0, 80)); }
}
res.readBack = [];
for (const c of await comps()) for (const p of await c.getAllPins())
  res.readBack.push(c.getState_Designator() + '.' + p.getState_PadNumber() + ' pad ' + (p.getState_Pad()[1] * 0.0254).toFixed(2) + ' hole ' + (p.getState_Hole()[1] * 0.254).toFixed(2) + ' mm');
await eda.pcb_Document.save();
return JSON.stringify(res);
