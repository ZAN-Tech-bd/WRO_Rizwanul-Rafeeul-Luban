// GENERATED from 21_place_pcb.template.js by layout_plan.py (--js). Edit the floor plan in layout_plan.py, not here.
// Robot carrier v0.5 PCB — step P5b: board outline (layer 11), mounting holes, placement of every footprint.
// Each part: [designator, x, y, rotation (deg, CCW as in layout_plan.py), [[pad, expectedX, expectedY], ...]].
// After setting x/y/rotation the pads are read back and compared with the expected positions (+/-1.5 mil); if they
// don't match, other rotations are tried (EasyEDA's rotation sense, 180/90/270) and the one that fits is reported.
// Idempotent: parts already in place are skipped; the outline and the H1..H9 holes are created when missing and replaced when they differ from the plan. Time-boxed (~20 s per run,
// bridge limit 30 s): re-run until "remaining": 0. Refuses to run unless project WRO_Robot is open.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\21_place_pcb.js"
const W = 3740, H = 3840;
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
  ['C10', 2290, 2300, 90, [["1", 2290.0, 2201.6], ["2", 2290.0, 2398.4]]],
  ['C11', 1240, 3560, 180, [["1", 1338.4, 3560.0], ["2", 1141.6, 3560.0]]],
  ['C12', 1690, 3165, 0, [["1", 1591.6, 3165.0], ["2", 1788.4, 3165.0]]],
  ['C13', 1350, 1950, 270, [["1", 1350.0, 2048.4], ["2", 1350.0, 1851.6]]],
  ['C14', 1350, 2280, 90, [["1", 1350.0, 2181.6], ["2", 1350.0, 2378.4]]],
  ['C3', 1220, 1255, 0, [["2", 1288.9, 1255.0], ["1", 1151.1, 1255.0]]],
  ['C4', 1730, 2050, 90, [["1", 1730.0, 1951.6], ["2", 1730.0, 2148.4]]],
  ['C5', 2960, 480, 0, [["2", 3028.9, 480.0], ["1", 2891.1, 480.0]]],
  ['C6', 2960, 720, 0, [["1", 2861.6, 720.0], ["2", 3058.4, 720.0]]],
  ['C7', 150, 1780, 90, [["1", 150.0, 1740.6], ["2", 150.0, 1819.4]]],
  ['C8', 150, 2030, 90, [["1", 150.0, 1931.6], ["2", 150.0, 2128.4]]],
  ['C9', 2790, 2422, 0, [["1", 2691.6, 2422.0], ["2", 2888.4, 2422.0]]],
  ['D1', 3050, 1702, 90, [["1", 3050.0, 1530.7], ["2", 3050.0, 1873.3]]],
  ['D2', 2840, 1702, 270, [["1", 2840.0, 1873.3], ["2", 2840.0, 1530.7]]],
  ['J1', 3217, 3245, 180, [["1", 3358.7, 3245.0], ["2", 3075.3, 3245.0]]],
  ['J10', 2700, 3699, 0, [["1", 2847.6, 3699.0], ["4", 2552.4, 3699.0]]],
  ['J11', 2137, 3735, 0, [["1", 2038.6, 3735.0], ["3", 2235.4, 3735.0]]],
  ['J12', 1475, 220, 90, [["1", 1475.0, 269.2], ["2", 1475.0, 170.8]]],
  ['J13', 1475, 600, 90, [["1", 1475.0, 649.2], ["2", 1475.0, 550.8]]],
  ['J14', 150, 2400, 90, [["1", 150.0, 2250.0], ["4", 150.0, 2550.0]]],
  ['J15', 3225, 3735, 0, [["1", 3126.6, 3735.0], ["3", 3323.4, 3735.0]]],
  ['J2', 3278.5, 2604, 90, [["1", 3278.5, 2505.5], ["2", 3278.5, 2702.4]]],
  ['J3', 3278.5, 2002, 90, [["1", 3278.5, 1903.6], ["2", 3278.5, 2100.4]]],
  ['J4', 3414, 1404, 90, [["1", 3414.0, 1305.6], ["2", 3414.0, 1502.5]]],
  ['J5', 2850, 2870, 180, [["1", 2929.0, 2870.0], ["2", 2771.0, 2870.0]]],
  ['J6', 150, 3105, 90, [["1", 150.0, 3449.5], ["8", 150.0, 2760.5]]],
  ['J7', 150, 1250, 270, [["1", 150.0, 1545.3], ["7", 150.0, 954.7]]],
  ['J8', 2980, 170, 0, [["1", 2881.6, 170.0], ["3", 3078.4, 170.0]]],
  ['J9', 1320, 920, 180, [["1", 1172.4, 920.0], ["4", 1467.6, 920.0]]],
  ['JP1', 2420, 2312, 90, [["1", 2420.0, 2212.0], ["3", 2420.0, 2412.0]]],
  ['LED1', 2900, 1210, 0, [["1", 2850.0, 1210.0], ["2", 2950.0, 1210.0]]],
  ['LED2', 2900, 940, 0, [["1", 2850.0, 940.0], ["2", 2950.0, 940.0]]],
  ['R10', 780, 2765, 0, [["1", 577.2, 2765.0], ["2", 982.8, 2765.0]]],
  ['R11', 1660, 1100, 270, [["1", 1660.0, 1302.8], ["2", 1660.0, 897.2]]],
  ['R12', 1780, 1100, 90, [["1", 1780.0, 897.2], ["2", 1780.0, 1302.8]]],
  ['R13', 1900, 1100, 270, [["1", 1900.0, 1302.8], ["2", 1900.0, 897.2]]],
  ['R14', 2680, 3295, 90, [["1", 2680.0, 3092.2], ["2", 2680.0, 3497.8]]],
  ['R15', 2552, 3295, 90, [["1", 2552.0, 3092.2], ["2", 2552.0, 3497.8]]],
  ['R16', 1790, 2965, 180, [["1", 1992.8, 2965.0], ["2", 1587.2, 2965.0]]],
  ['R17', 2264, 3065, 180, [["1", 2466.8, 3065.0], ["2", 2061.2, 3065.0]]],
  ['R18', 2310, 3360, 90, [["1", 2310.0, 3157.2], ["2", 2310.0, 3562.8]]],
  ['R19', 1790, 3365, 180, [["1", 1992.8, 3365.0], ["2", 1587.2, 3365.0]]],
  ['R2', 780, 3465, 0, [["1", 577.2, 3465.0], ["2", 982.8, 3465.0]]],
  ['R20', 1780, 608, 90, [["1", 1780.0, 405.2], ["2", 1780.0, 810.8]]],
  ['R21', 1660, 608, 90, [["1", 1660.0, 405.2], ["2", 1660.0, 810.8]]],
  ['R22', 2020, 608, 90, [["1", 2020.0, 405.2], ["2", 2020.0, 810.8]]],
  ['R23', 1900, 608, 90, [["1", 1900.0, 405.2], ["2", 1900.0, 810.8]]],
  ['R24', 1230, 3262, 90, [["1", 1230.0, 3059.2], ["2", 1230.0, 3464.8]]],
  ['R25', 1350, 3262, 270, [["1", 1350.0, 3464.8], ["2", 1350.0, 3059.2]]],
  ['R26', 2500, 1210, 0, [["1", 2297.2, 1210.0], ["2", 2702.8, 1210.0]]],
  ['R27', 2500, 940, 0, [["1", 2297.2, 940.0], ["2", 2702.8, 940.0]]],
  ['R3', 780, 2965, 0, [["1", 577.2, 2965.0], ["2", 982.8, 2965.0]]],
  ['R4', 760, 1300, 270, [["1", 760.0, 1502.8], ["2", 760.0, 1097.2]]],
  ['R5', 520, 1300, 270, [["1", 520.0, 1502.8], ["2", 520.0, 1097.2]]],
  ['R6', 640, 1300, 270, [["1", 640.0, 1502.8], ["2", 640.0, 1097.2]]],
  ['R7', 780, 1865, 0, [["1", 577.2, 1865.0], ["2", 982.8, 1865.0]]],
  ['R8', 1870, 2195, 90, [["1", 1870.0, 1992.2], ["2", 1870.0, 2397.8]]],
  ['R9', 1610, 2195, 270, [["1", 1610.0, 2397.8], ["2", 1610.0, 1992.2]]],
  ['SW1', 1191, 220, 90, [["4", 1102.4, 92.0], ["2", 1279.6, 348.0]]],
  ['SW2', 1191, 600, 90, [["4", 1102.4, 472.0], ["2", 1279.6, 728.0]]],
  ['U1A', 470, 2715, 270, [["1", 470.0, 3765.0], ["22", 470.0, 1665.0]]],
  ['U1B', 1470, 2715, 270, [["1", 1470.0, 3765.0], ["22", 1470.0, 1665.0]]],
  ['U2A', 1954, 1840, 0, [["8", 2304.0, 1840.0], ["1", 1604.0, 1840.0]]],
  ['U2B', 1954, 2548.66, 0, [["8", 2304.0, 2548.66], ["1", 1604.0, 2548.66]]],
  ['U3', 2490, 1595, 180, [["1", 2640.0, 1595.0], ["4", 2340.0, 1595.0]]],
  ['U4', 1880, 1595, 180, [["1", 2030.0, 1595.0], ["4", 1730.0, 1595.0]]],
];
// [name, x, y, drill, pad, plated]
const HOLES = [
  ['H1', 157, 157, 126, 236, true],
  ['H2', 3583, 157, 126, 236, true],
  ['H3', 157, 3683, 126, 236, true],
  ['H4', 3583, 3683, 126, 236, true]
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
