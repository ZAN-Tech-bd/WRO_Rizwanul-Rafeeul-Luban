// GENERATED from 21_place_pcb.template.js by layout_plan.py (--js). Edit the floor plan in layout_plan.py, not here.
// Robot carrier v0.5 PCB — step P5b: board outline (layer 11), mounting holes, placement of every footprint.
// Each part: [designator, x, y, rotation (deg, CCW as in layout_plan.py), [[pad, expectedX, expectedY], ...]].
// After setting x/y/rotation the pads are read back and compared with the expected positions (+/-1.5 mil); if they
// don't match, other rotations are tried (EasyEDA's rotation sense, 180/90/270) and the one that fits is reported.
// Idempotent: parts already in place are skipped; the outline and the H1..H9 holes are created when missing and replaced when they differ from the plan. Time-boxed (~20 s per run,
// bridge limit 30 s): re-run until "remaining": 0. Refuses to run unless project WRO_Robot is open.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\21_place_pcb.js"
const W = /*W*/, H = /*H*/;
const PROJECT = 'WRO_Robot';
// No timers: EasyEDA freezes its page timers while its window is hidden (2026-10-14), so 'sleep' yields through
// cheap API calls instead.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT, project: proj && (proj.friendlyName || proj.name) });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open in ' + PROJECT });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(800);

const P = [
/*PLAN*/
];
// [name, x, y, drill, pad, plated]
const HOLES = [
  /*HOLES*/
];

const t0 = Date.now();
const res = { outline: '', holes: 0, holesExisting: 0, placed: 0, alreadyOk: 0, remaining: 0, rotFixed: [], failed: [], missing: [] };

// ---- outline (layer 11): ONE closed polyline, created if missing, replaced if its extents differ from W x H ----
// EasyEDA 3.2.206 (2026-10-25): pcb_PrimitiveLine only creates copper (layers 3 / 11 are refused), and the v0.4 outline
// made of four layer-11 lines was gone after the update; outline and silk lines are pcb_PrimitivePolyline now.
// Legacy layer-11 lines (older builds) are removed so the board never carries two outlines.
const legacy = ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => String(l.getState_Layer()) === '11');
if (legacy.length) { await eda.pcb_PrimitiveLine.delete(legacy.map((l) => l.getState_PrimitiveId())); res.outlineLegacyRemoved = legacy.length; }
const srcOf = (p) => { try { return p.getState_Polygon().getSource(); } catch (e) { return null; } };
const extOf = (p) => {
  const n = (srcOf(p) || []).filter((v) => typeof v === 'number');
  const xs = n.filter((_, i) => i % 2 === 0), ys = n.filter((_, i) => i % 2 === 1);
  return xs.length ? [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)] : null;
};
let outl = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((p) => String(p.getState_Layer()) === '11');
const okExt = (e) => e && Math.abs(e[0]) < 0.5 && Math.abs(e[1]) < 0.5 && Math.abs(e[2] - W) < 0.5 && Math.abs(e[3] - H) < 0.5;
if (outl.length === 1 && okExt(extOf(outl[0]))) res.outline = 'exists ' + W + ' x ' + H;
else {
  if (outl.length) {
    await eda.pcb_PrimitivePolyline.delete(outl.map((p) => p.getState_PrimitiveId()));
    res.outlineRemoved = outl.length + ' polylines ' + outl.map((p) => (extOf(p) || []).map(Math.round).join(',')).join(' | ');
  }
  const poly = eda.pcb_MathPolygon.createPolygon([0, 0, 'L', W, 0, W, H, 0, H, 0, 0]);
  const o = await eda.pcb_PrimitivePolyline.create('', 11, poly, 10);
  res.outline = o ? 'created ' + W + ' x ' + H : 'create returned nothing';
}
// ---- mounting holes = free pads named H1..H9: remove the ones not in the plan (moved / resized), add missing ----
const freePads = ((await eda.pcb_PrimitivePad.getAll()) || []).filter((p) => /^(H|Z)\d+$/.test(String(p.getState_PadNumber())));
const want = (p) => HOLES.find(([n, hx, hy, drill, pad]) => n === String(p.getState_PadNumber()) && Math.abs(p.getState_X() - hx) < 1 &&
  Math.abs(p.getState_Y() - hy) < 1 && Math.abs(((p.getState_Pad() || [])[1] || 0) - pad) < 1);
const stale = freePads.filter((p) => !want(p));
if (stale.length) { await eda.pcb_PrimitivePad.delete(stale.map((p) => p.getState_PrimitiveId())); res.holesRemoved = stale.map((p) => p.getState_PadNumber()).join(','); }
const keep = freePads.filter((p) => want(p));
for (const [name, hx, hy, drill, pad, plated] of HOLES) {
  if (keep.some((p) => String(p.getState_PadNumber()) === name)) { res.holesExisting++; continue; }
  await eda.pcb_PrimitivePad.create(12, name, hx, hy, 0, ['ELLIPSE', pad, pad], '', ['ROUND', drill], 0, 0, 0, plated, 0);
  res.holes++;
}

// ---- placement ----
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids);
const byDes = {};
for (const c of comps) byDes[c.getState_Designator()] = c;
const padsOf = async (id) => {
  const c = (await eda.pcb_PrimitiveComponent.get([id]))[0];
  const o = {};
  for (const p of (await c.getAllPins()) || []) o[String(p.getState_PadNumber())] = { x: p.getState_X(), y: p.getState_Y() };
  return o;
};
const fits = (pads, chk) => chk.every(([n, x, y]) => pads[n] && Math.abs(pads[n].x - x) < 1.5 && Math.abs(pads[n].y - y) < 1.5);
for (const [des, x, y, rot, chk] of P) {
  const c = byDes[des];
  if (!c) { res.missing.push(des); continue; }
  const id = c.getState_PrimitiveId();
  if (fits(await padsOf(id), chk)) { res.alreadyOk++; continue; }
  if (Date.now() - t0 > 20000) { res.remaining++; continue; }
  let ok = false;
  for (const r of [...new Set([rot, (360 - rot) % 360, (rot + 180) % 360, (rot + 90) % 360, (rot + 270) % 360])]) {
    const a = (await eda.pcb_PrimitiveComponent.get([id]))[0].toAsync();
    a.setState_X(x); a.setState_Y(y); a.setState_Rotation(r);
    await a.done();
    if (fits(await padsOf(id), chk)) { ok = true; if (r !== rot) res.rotFixed.push(des + ':' + rot + '->' + r); break; }
  }
  if (ok) res.placed++; else res.failed.push(des);
}
await sleep(800);
await eda.pcb_Document.save();
return JSON.stringify(res);
