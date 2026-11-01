// GENERATED from 21b_place_pcb.template.js by layout_plan.py (--js). Edit the floor plan in layout_plan.py, not here.
// Hub v0.4 PCB — re-placement 2026-10-26 (user: shorter board): board outline (layer 11), 4 M3 holes, every footprint.
// From robot/21_place_pcb.template.js. Each part: [designator, x, y, rotation (deg, CCW), {pad: [expectedX, expectedY]}].
// After setting x/y/rotation the pads are read back and compared with the plan (+/-1.5 mil); if they don't match,
// other rotations are tried and the one that fits is reported. Idempotent: parts already in place are skipped; the
// outline is replaced when its extents differ; the corner holes (free pads named "MH", v0.3) are moved to the plan.
// Time-boxed (~18 s): re-run until "remaining": 0. Gates: WRO_HUB, PCB tab, visible.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\21b_place_pcb.js"
const W = /*W*/, H = /*H*/;
const PROJECT = 'WRO_HUB';
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open in ' + PROJECT });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(600);

const P = /*PLAN*/;
const HOLES = [[170, 170], [W - 170, 170], [170, H - 170], [W - 170, H - 170]];
const t0 = Date.now();
const res = { outline: '', holesMoved: 0, holesOk: 0, placed: 0, alreadyOk: 0, remaining: 0, rotFixed: [], failed: [], missing: [] };

// ---- outline: ONE closed layer-11 polyline of W x H ----
const srcOf = (p) => { try { return p.getState_Polygon().getSource(); } catch (e) { return null; } };
const extOf = (p) => {
  const n = (srcOf(p) || []).filter((v) => typeof v === 'number');
  const xs = n.filter((_, i) => i % 2 === 0), ys = n.filter((_, i) => i % 2 === 1);
  return xs.length ? [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)] : null;
};
const outl = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((p) => String(p.getState_Layer()) === '11');
const okExt = (e) => e && Math.abs(e[0]) < 0.5 && Math.abs(e[1]) < 0.5 && Math.abs(e[2] - W) < 0.5 && Math.abs(e[3] - H) < 0.5;
if (outl.length === 1 && okExt(extOf(outl[0]))) res.outline = 'exists ' + W + ' x ' + H;
else {
  if (outl.length) {
    res.outlineRemoved = outl.map((p) => (extOf(p) || []).map(Math.round).join(',')).join(' | ');
    await eda.pcb_PrimitivePolyline.delete(outl.map((p) => p.getState_PrimitiveId()));
  }
  const o = await eda.pcb_PrimitivePolyline.create('', 11, eda.pcb_MathPolygon.createPolygon([0, 0, 'L', W, 0, W, H, 0, H, 0, 0]), 10);
  res.outline = o ? 'created ' + W + ' x ' + H : 'create returned nothing';
}
// ---- M3 holes: free pads "MH"; each planned spot keeps one, stray ones are moved onto the free spots ----
const mh = ((await eda.pcb_PrimitivePad.getAll()) || []).filter((p) => String(p.getState_PadNumber()) === 'MH');
const at = (p, [hx, hy]) => Math.abs(p.getState_X() - hx) < 1 && Math.abs(p.getState_Y() - hy) < 1;
const free = HOLES.filter((h) => !mh.some((p) => at(p, h)));
const stray = mh.filter((p) => !HOLES.some((h) => at(p, h)));
res.holesOk = HOLES.length - free.length;
if (stray.length !== free.length) return JSON.stringify({ abort: mh.length + ' MH pads, ' + stray.length + ' stray vs ' + free.length + ' free spots' });
for (let i = 0; i < stray.length; i++) {
  const a = stray[i].toAsync(); a.setState_X(free[i][0]); a.setState_Y(free[i][1]); await a.done(); res.holesMoved++;
}

// ---- placement ----
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const byDes = {};
for (const c of comps) byDes[c.getState_Designator()] = c;
const padsOf = async (id) => {
  const c = (await eda.pcb_PrimitiveComponent.get([id]))[0];
  const o = {};
  for (const p of (await c.getAllPins()) || []) o[String(p.getState_PadNumber())] = [p.getState_X(), p.getState_Y()];
  return o;
};
const fits = (pads, chk) => Object.entries(chk).every(([n, [x, y]]) => pads[n] && Math.abs(pads[n][0] - x) < 1.5 && Math.abs(pads[n][1] - y) < 1.5);
for (const [des, x, y, rot, chk] of P) {
  const c = byDes[des];
  if (!c) { res.missing.push(des); continue; }
  const id = c.getState_PrimitiveId();
  if (fits(await padsOf(id), chk)) { res.alreadyOk++; continue; }
  if (Date.now() - t0 > 18000) { res.remaining++; continue; }
  let ok = false;
  for (const r of [...new Set([rot, (360 - rot) % 360, (rot + 180) % 360, (rot + 90) % 360, (rot + 270) % 360])]) {
    const a = (await eda.pcb_PrimitiveComponent.get([id]))[0].toAsync();
    a.setState_X(x); a.setState_Y(y); a.setState_Rotation(r);
    await a.done();
    if (fits(await padsOf(id), chk)) { ok = true; if (r !== rot) res.rotFixed.push(des + ':' + rot + '->' + r); break; }
  }
  if (ok) res.placed++; else res.failed.push(des);
}
await sleep(600);
await eda.pcb_Document.save();
return JSON.stringify(res);
