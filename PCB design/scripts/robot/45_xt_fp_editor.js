// Robot v0.6 — D-031 made persistent: XT pad holes in the in-place FOOTPRINT EDITOR (WRITES the open footprint-editor
// document only; does NOT save, does NOT touch PCB1). Why: 44_xt_pads.js edits component pads on the canvas, but the
// saved project kept the library drills (export 2026-10-27: XT60 4.6 mm, XT30 2.2 / 1.8 mm) — same as hub H-001.
// The hub proved the persistent route: the user opens the part's footprint editor, pcb_PrimitivePad.modify changes
// the existing pads (ids kept -> PAD_NET rows keep their nets), the user saves the tab.
// Steps (user, per part J1, J2, J3, J4): PCB1 -> right-click the part -> Edit Footprint -> run this -> save the tab.
// Targets: XT60 (2 pads 7.2 mm apart): pad 5.5 mm round, hole 4.0 mm; XT30 (2 pads 5.0 mm apart): pad 3.5 mm, hole 2.5 mm.
// Free pads in this editor report the hole in mil (hub lesson), so the hole is compared and written in mil.
// Idempotent: pads already at target are skipped. Readback = the native document source (what save serialises).
// NOTE: Import Changes from the schematic re-binds the library footprint (the device still names it): re-run after it.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\45_xt_fp_editor.js"
const PROJECT = 'WRO_Robot';
const MM = 1 / 0.0254;
const KINDS = [{ name: 'XT60', pitch: 7.2, pad: 5.5, hole: 4.0 }, { name: 'XT30', pitch: 5.0, pad: 3.5, hole: 2.5 }];
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (!proj || proj.friendlyName !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (!doc || doc.documentType !== 4) return JSON.stringify({ abort: 'active tab is not a footprint editor (right-click the XT part -> Edit Footprint)', doc });
const near = (a, b, t = 0.05) => Math.abs(a - b) < t;
const pads = await eda.pcb_PrimitivePad.getAll();
if (pads.length !== 2) return JSON.stringify({ abort: pads.length + ' pads, expected 2 (XT connector)' });
const pitch = Math.hypot(pads[0].getState_X() - pads[1].getState_X(), pads[0].getState_Y() - pads[1].getState_Y()) / MM;
const kind = KINDS.find((k) => near(pitch, k.pitch, 0.15));
if (!kind) return JSON.stringify({ abort: 'pad pitch ' + pitch.toFixed(2) + ' mm is neither XT60 (7.2) nor XT30 (5.0)' });
const PAD = ['ELLIPSE', kind.pad * MM, kind.pad * MM], HOLE = ['ROUND', kind.hole * MM, kind.hole * MM];
const res = { doc: doc.uuid, kind: kind.name, padsChanged: [], padsOk: [], errors: [] };
for (const p of pads) {
  const id = p.getState_PrimitiveId(), s = p.getState_Pad(), h = p.getState_Hole();
  if (s && near(s[1], PAD[1]) && near(s[2], PAD[2]) && h && h[0] === 'ROUND' && near(h[1], HOLE[1])) { res.padsOk.push(id); continue; }
  try {
    await eda.pcb_PrimitivePad.modify(id, { pad: PAD, hole: HOLE, metallization: true });
    res.padsChanged.push(id);
  } catch (e) { res.errors.push(id + ' ' + String(e).slice(0, 100)); }
}
const src = await eda.sys_FileManager.getDocumentSource();
const recs = src.split('\n').map((l) => { const i = l.indexOf('||'); if (i < 0) return null;
  try { return [JSON.parse(l.slice(0, i)), JSON.parse(l.slice(i + 2).replace(/\|\s*$/, ''))]; } catch (e) { return null; } }).filter(Boolean);
res.readBack = recs.filter(([a]) => a.type === 'PAD').map(([a, b]) => ({ id: a.id, num: b.num,
  pad: [b.defaultPad.padType, +(b.defaultPad.width * 0.0254).toFixed(3), +(b.defaultPad.height * 0.0254).toFixed(3)],
  hole: [b.hole && b.hole.holeType, +(b.hole.width * 0.0254).toFixed(3), +(b.hole.height * 0.0254).toFixed(3)], plated: b.plated }));
return JSON.stringify(res);
