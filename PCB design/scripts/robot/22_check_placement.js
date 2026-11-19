// Robot carrier v0.5 PCB — step P5c: verify the placement inside EasyEDA (READ-ONLY).
// Real footprint bboxes (pcb_Primitive.getPrimitivesBBox) against the floor-plan rules of layout_plan.py (constants below
// were printed from it on 2026-10-16, v0.5 sets updated 2026-10-25; re-print them if the plan changes): overlaps, parts off the board (XT fronts may
// overhang 15 mil), M3 hole clearance, antenna strip, METAL 15 mm, USB lane, height classes under the raised modules,
// LM386 foam spots; module socket geometry (DevKit rows 25.40 mm + pin 1 at the top edge, DFPlayer rows 18.00 mm,
// LM386 VCC side); polarity / power nets on the pads. Pattern: attendance 22_check_placement.js.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\22_check_placement.js"
const PROJECT = 'WRO_Robot';
const R = {"W": 3740, "H": 3840, "ANT": [416, 3640, 1524, 3840], "METAL15": [26, 3250, 1914, 3840], "LANE": [400, 0, 1046, 1590],
  "MODULES": [["DevKit", [420, 1590, 1520, 3840]], ["DFPlayer", [1560, 1801, 2348, 2587.66]], ["LM386 R", [1606.5, 40, 2153.5, 1650]], ["LM386 L", [2216.5, 40, 2763.5, 1650]]],
  "OWN": {"DevKit": ["U1A", "U1B"], "DFPlayer": ["U2A", "U2B"], "LM386 R": ["U4"], "LM386 L": ["U3"]},
  "FOAM": [[1700, 60, 2060, 360], [2310, 60, 2670, 360]],
  "FLAT": ["D1", "D2", "R10", "R11", "R12", "R13", "R14", "R15", "R16", "R17", "R18", "R19", "R2", "R20", "R21", "R22", "R23", "R24", "R25", "R26", "R27", "R3", "R4", "R5", "R6", "R7", "R8", "R9"],
  "LOW": ["C10", "C11", "C12", "C13", "C14", "C4", "C6", "C8", "D1", "D2", "R10", "R11", "R12", "R13", "R14", "R15", "R16", "R17", "R18", "R19", "R2", "R20", "R21", "R22", "R23", "R24", "R25", "R26", "R27", "R3", "R4", "R5", "R6", "R7", "R8", "R9"],
  "PADS_ONLY": ["J14"], "REACH": ["J14", "JP1", "SW1", "SW2"],
  "METAL": ["C3", "C5", "C7", "C9", "J1", "J2", "J3", "J4", "JP1"],
  "PIN1_Y": 3765, "U1A_X": 470, "U1B_X": 1470, "DF_CX": 1954, "DF_Y0": 1840, "DF_ROW": 708.66, "AMP_SOCK_Y": 1595};
// polarity and supply pins that must carry these nets (spec §5)
const NETS = [['J1', '1', 'GND'], ['J1', '2', 'BAT+'], ['J5', '1', 'BAT+'], ['J5', '2', 'BAT_SW'], ['J2', '1', 'BAT_SW'], ['J2', '2', 'GND'],
  ['J3', '1', 'BAT_SW'], ['J3', '2', 'GND'], ['J4', '1', 'GND'], ['J4', '2', '+5V'], ['D1', '1', 'AMP_D'], ['D1', '2', 'BAT_SW'],
  ['D2', '1', 'VAMP_BAT'], ['D2', '2', 'AMP_D'], ['C3', '1', '+5V'],
  ['C5', '1', '+5V'], ['C7', '1', '+3V3'], ['C9', '1', 'VAMP'], ['LED1', '1', 'LED1_A'], ['LED2', '1', 'LED2_A'],
  ['U1A', '1', '+3V3'], ['U1A', '21', '+5V'], ['U1A', '22', 'GND'], ['U1B', '1', 'GND'], ['U2A', '1', '+5V'], ['U2A', '7', 'GND'],
  ['U3', '1', 'VAMP'], ['U3', '2', 'DAC_L'], ['U4', '1', 'VAMP'], ['U4', '2', 'DAC_R'], ['JP1', '1', 'VAMP_BAT'], ['JP1', '2', 'VAMP'],
  ['JP1', '3', '+5V'], ['J9', '1', '+5V'], ['J10', '1', '+3V3'], ['J11', '1', '+3V3'], ['J15', '1', '+3V3'], ['J15', '3', 'OBST'], ['J11', '3', 'OBST'], ['J7', '1', '+3V3'], ['J8', '1', '+5V'], ['J6', '1', '+5V']];
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
const t0 = Date.now(); while (Date.now() - t0 < 600) await eda.dmt_Project.getCurrentProjectInfo();

// board extents from the layer-11 outline: a closed polyline since EasyEDA 3.2.206 (2026-10-25: pcb_PrimitiveLine is
// copper-only now); legacy layer-11 lines of older builds are read too
const _src = (p) => { try { return p.getState_Polygon().getSource() || []; } catch (e) { return []; } };
const _xs = [], _ys = [];
for (const p of ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((p) => String(p.getState_Layer()) === '11')) {
  _src(p).filter((v) => typeof v === 'number').forEach((v, i) => (i % 2 ? _ys : _xs).push(v));
}
for (const l of ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => String(l.getState_Layer()) === '11')) {
  _xs.push(l.getState_StartX(), l.getState_EndX()); _ys.push(l.getState_StartY(), l.getState_EndY());
}
const W = Math.max(..._xs);
const H = Math.max(..._ys);
const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const rect = {}, pad = {}, net = {};
for (const c of comps) {
  const d = c.getState_Designator();
  const b = await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]);
  rect[d] = [b.minX, b.minY, b.maxX, b.maxY];
  pad[d] = {}; net[d] = {};
  for (const p of (await c.getAllPins()) || []) {
    const n = String(p.getState_PadNumber());
    pad[d][n] = [p.getState_X(), p.getState_Y()];
    net[d][n] = p.getState_Net() || '';
  }
}
const ov = (a, b) => Math.min(a[2], b[2]) - Math.max(a[0], b[0]) > 0.5 && Math.min(a[3], b[3]) - Math.max(a[1], b[1]) > 0.5;
const des = Object.keys(rect).sort();
const has = (k, d) => R[k].includes(d);
const res = { board: W + ' x ' + H, parts: des.length, overlaps: [], offBoard: [], nearHoles: [], antenna: [], metal15: [], usbLane: [],
  underModule: [], foam: [], nets: [], geometry: {} };
for (let i = 0; i < des.length; i++) for (let j = i + 1; j < des.length; j++) if (ov(rect[des[i]], rect[des[j]])) res.overlaps.push(des[i] + '~' + des[j]);
for (const d of des) {
  const r = rect[d], o = /^J[1-4]$/.test(d) ? 15 : 0;
  if (r[0] < -0.5 || r[1] < -0.5 || r[2] > W + o + 0.5 || r[3] > H + 0.5) res.offBoard.push(d + ':' + r.map(Math.round).join(','));
  if (!['U1A', 'U1B'].includes(d) && ov(r, R.ANT)) res.antenna.push(d);
  if (has('METAL', d) && ov(r, R.METAL15)) res.metal15.push(d);
  if (!has('LOW', d) && !has('PADS_ONLY', d) && ov(r, R.LANE)) res.usbLane.push(d);
  for (const [m, mr] of R.MODULES) {
    if (R.OWN[m].includes(d)) continue;
    const ok = m.startsWith('LM386') ? R.FLAT : R.LOW;
    if ((!ok.includes(d) || has('REACH', d)) && ov(r, mr)) res.underModule.push(d + '@' + m);
  }
  if (R.FOAM.some((f) => ov(r, f))) res.foam.push(d);
}
// mounting holes = free pads named H1..H4 (component pads are filtered out by name)
const holes = ((await eda.pcb_PrimitivePad.getAll()) || []).map((p) => ({ n: String(p.getState_PadNumber()), x: p.getState_X(), y: p.getState_Y() }))
  .filter((h) => /^H\d+$/.test(h.n));
for (const h of holes) for (const d of des) {
  const r = rect[d], dist = Math.hypot(Math.max(r[0] - h.x, 0, h.x - r[2]), Math.max(r[1] - h.y, 0, h.y - r[3]));
  if (dist < 126) res.nearHoles.push(d + '@' + h.n + ':' + Math.round(dist));
}
res.holes = holes.map((h) => h.n + '(' + Math.round(h.x) + ',' + Math.round(h.y) + ')').sort().join(' ');
for (const [d, n, want] of NETS) if (!net[d] || net[d][n] !== want) res.nets.push(d + '.' + n + '=' + (net[d] ? net[d][n] : '?') + ' (want ' + want + ')');
const P = (d, n) => pad[d] && pad[d][n];
const mm = (v) => +(v * 0.0254).toFixed(3);
res.geometry = {
  devkitRows_mm: mm(P('U1B', '1')[0] - P('U1A', '1')[0]),                      // 25.400
  devkitPin1Level: +(P('U1B', '1')[1] - P('U1A', '1')[1]).toFixed(2),          // 0
  devkitPin1BelowTop_mm: mm(H - P('U1A', '1')[1]),                             // 1.905 (DevKit top flush with the edge)
  devkitPin1AtTop: P('U1A', '1')[1] > P('U1A', '22')[1] && P('U1B', '1')[1] > P('U1B', '22')[1],
  devkitPitch_mm: mm(P('U1A', '1')[1] - P('U1A', '2')[1]),                    // 2.540
  dfRows_mm: mm(P('U2B', '1')[1] - P('U2A', '1')[1]),                          // 18.000
  dfPin1Column: +(P('U2B', '1')[0] - P('U2A', '1')[0]).toFixed(2),             // 0 (DF 1 and DF 16 on the left)
  dfPin1Left: P('U2A', '1')[0] < P('U2A', '8')[0] && P('U2B', '1')[0] < P('U2B', '8')[0],
  dfLeftGap_mm: mm(P('U2A', '1')[0] - 44 - (P('U1B', '1')[0] + 50)),          // DFPlayer edge to DevKit edge ~1.0
  lm386VccRight: P('U3', '1')[0] > P('U3', '4')[0] && P('U4', '1')[0] > P('U4', '4')[0],
  lm386SocketY: [P('U4', '1')[1], P('U3', '1')[1]],                            // 1595
  xtFrontsAtEdge: ['J1', 'J2', 'J3', 'J4'].map((d) => d + ':' + (rect[d][2] - W).toFixed(1)).join(' '),
};
return JSON.stringify(res, null, 1);
