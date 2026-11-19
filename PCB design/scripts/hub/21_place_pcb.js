// Hub v0.4 PCB — step 4b: board outline, 4 corner M3 holes, and placement of all 90 footprints.
// SUPERSEDED 2026-10-26: the user places the hub parts by hand (46_unplace_all.js). Do NOT re-run: it would undo that.
// Board 5200 x 3900 mil (132.1 x 99.1 mm), origin bottom-left, y up. Floor plan (left -> right):
//   power column (XT60/XT30 on the left edge, switch harness, LEDs, servo ports) | DevKit (antenna over the top
//   edge, USB lane below kept low) | middle column (E-stop, I2C, buzzer, C2 on top; SD module lying flat over the
//   74HCT125 + low parts, card slot at the bottom edge) | station block (2 x 4 identical cells: S1..S4 up col A,
//   S5..S8 down col B = LED chain order; zones Z1..Z4 = neighbouring pairs).
// Safety: refuses to run unless project WRO_HUB is open with its PCB tab. Rotation direction is verified from the
// pads after placing (each part has an orientation check; 180/90/270 alternatives are tried automatically).
// Idempotent: re-running moves parts to the same targets; outline/holes are only added if missing.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/21_place_pcb.js"
const W = 5200, H = 3900;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== 'WRO_HUB') return JSON.stringify({ abort: 'open project is not WRO_HUB', project: proj && (proj.friendlyName || proj.name) });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab open in WRO_HUB' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(800);

// ---- orientation checks (p = {padNumber: {x, y}}) ----
const facesLeft = (pw, mt) => (p) => Math.min(...pw.map((n) => p[n].x)) > Math.max(...mt.map((n) => p[n].x)); // body/mating face -x
const vertical = (a, b, topIsA) => (p) => Math.abs(p[a].x - p[b].x) < 1 && (topIsA ? p[a].y > p[b].y : true);
const horizontal = (a, b, leftIsA) => (p) => Math.abs(p[a].y - p[b].y) < 1 && (leftIsA === undefined || (leftIsA ? p[a].x < p[b].x : p[a].x > p[b].x));
const CHK = {
  XT60M: facesLeft(['1', '2'], ['3', '4']),
  XT60F: facesLeft(['3', '4'], ['1', '2']),
  XT30M: facesLeft(['1', '2'], ['3', '4']),
  VH6: vertical('1', '6', true),
  HDR22: vertical('1', '22', true),
  SD6: horizontal('1', '6', true),
  XH8: horizontal('1', '8', false), // pin 1 (+5V) on the right
  XH4: horizontal('1', '4', false),
  XH3H: horizontal('1', '3'),
  XH3V: vertical('1', '3'),
  H2: horizontal('1', '2'),
  V2: vertical('1', '2'),
  DIP: (p) => p['1'].x < p['7'].x && Math.abs(p['1'].y - p['7'].y) < 1 && p['14'].y > p['1'].y, // pin 1 bottom-left
  LED: horizontal('1', '2', true), // anode left
  TO92: horizontal('1', '3', true),
  XH2: horizontal('1', '2', false), // pin 1 (+5V) on the right, like the station ports
};

// ---- targets: [designator, x, y, rotation guess, check] ----
const P = [
  // power column
  ['J1', 330, 3150, 0, 'XT60M'], ['J2', 330, 2400, 0, 'XT60M'],
  ['J4', 330, 1650, 90, 'XT60F'], ['J5', 290, 900, 270, 'XT30M'],
  ['J3', 760, 2780, 270, 'VH6'],
  ['LED1', 450, 3650, 0, 'LED'], ['LED2', 720, 3650, 0, 'LED'], ['LED3', 750, 350, 0, 'LED'],
  ['R1', 1180, 3420, 90, 'V2'], ['R2', 1310, 3420, 90, 'V2'], ['R3', 750, 160, 0, 'H2'],
  ['J21', 720, 2040, 90, 'XH3V'], ['J22', 720, 1610, 90, 'XH3V'], ['J23', 720, 1180, 90, 'XH3V'], ['J24', 720, 750, 90, 'XH3V'],
  // DevKit (rows 1000 mil apart, pin 1 at the antenna/top end 75 mil below the board edge)
  ['U1A', 1050, 2775, 270, 'HDR22'], ['U1B', 2050, 2775, 270, 'HDR22'],
  // middle column, top: E-stop, I2C, buzzer, servo bulk
  ['J20', 2410, 3640, 0, 'XH3H'], ['J18', 2905, 3640, 0, 'XH4'],
  ['R13', 2410, 3360, 0, 'H2'], ['R15', 2905, 3360, 0, 'H2'], ['R16', 2905, 3230, 0, 'H2'],   // v0.4: R14 retired
  ['BZ1', 2400, 2800, 0, 'H2'], ['Q1', 2880, 2960, 0, 'TO92'],
  ['R11', 2905, 2800, 0, 'H2'], ['R12', 2905, 2670, 0, 'H2'], ['D1', 2905, 2540, 0, 'H2'],
  ['C2', 2905, 2250, 0, 'H2'],
  // v0.4 spare 5 V outputs (user): side by side in the free strip between the DevKit and C2, above the SD module
  ['J25', 2460, 1920, 0, 'XH2'], ['J26', 2880, 1920, 0, 'XH2'],
  // middle column, bottom: SD header (module lies over the parts below, card slot at the bottom edge)
  ['U6', 2650, 1720, 0, 'SD6'],
  ['U2', 2650, 1300, 0, 'DIP'], ['C1', 2450, 1590, 0, 'H2'],
  ['R4', 2410, 960, 0, 'H2'], ['R5', 2890, 960, 0, 'H2'], ['R6', 2410, 830, 0, 'H2'],
  // servo series resistors next to their driver (74HCT125 2Y), also under the module
  ['R7', 2890, 830, 0, 'H2'], ['R8', 2410, 700, 0, 'H2'], ['R9', 2890, 700, 0, 'H2'], ['R10', 2410, 570, 0, 'H2'],
];
// station block: S1..S4 up column A, S5..S8 down column B
const COLA = 3690, COLB = 4666, ROWS = [860, 1727, 2593, 3440];
const CELL = { 1: [COLA, 0], 2: [COLA, 1], 3: [COLA, 2], 4: [COLA, 3], 5: [COLB, 3], 6: [COLB, 2], 7: [COLB, 1], 8: [COLB, 0] };
for (let k = 1; k <= 8; k++) {
  const cx = CELL[k][0], cy = ROWS[CELL[k][1]];
  P.push(['J' + (9 + k), cx, cy, 0, 'XH8']);
  if (k > 7) continue;
  // v0.4: no DHT pull-up (R21–R27 retired; the module has its own)
  P.push(['R' + (30 + k), cx + 240, cy - 220, 0, 'H2']); // DHT series
  P.push(['C' + (2 + k), cx - 340, cy - 350, 0, 'H2']); // MQ filter
  P.push(['R' + (60 + k), cx + 60, cy - 350, 0, 'H2']); // LED hop to the next station
  if (k % 2) {
    const n = (k + 1) / 2;
    P.push(['R' + (40 + n), cx - 240, cy - 480, 0, 'H2']); // zone pull-up
    P.push(['R' + (50 + n), cx + 240, cy - 480, 0, 'H2']); // zone series
    P.push(['SB' + k, cx - 360, cy - 610, 0, 'H2']);
  } else {
    P.push(['SB' + k, cx - 360, cy - 480, 0, 'H2']);
  }
}

// ---- place ----
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
const t0 = Date.now();
const res = { placed: 0, alreadyOk: 0, remaining: 0, rotFixed: [], failed: [], missing: [] };
for (const [des, x, y, rot, chk] of P) {
  const c = byDes[des];
  if (!c) { res.missing.push(des); continue; }
  const id = c.getState_PrimitiveId();
  // idempotent: already at the target with a passing orientation -> skip
  if (Math.abs(c.getState_X() - x) < 0.5 && Math.abs(c.getState_Y() - y) < 0.5 && CHK[chk](await padsOf(id))) { res.alreadyOk++; continue; }
  if (Date.now() - t0 > 20000) { res.remaining++; continue; } // bridge call limit is 30 s: re-run to continue
  let ok = false;
  for (const r of [rot, (rot + 180) % 360, (rot + 90) % 360, (rot + 270) % 360]) {
    const cur = (await eda.pcb_PrimitiveComponent.get([id]))[0];
    const a = cur.toAsync();
    a.setState_X(x); a.setState_Y(y); a.setState_Rotation(r);
    await a.done();
    if (CHK[chk](await padsOf(id))) { ok = true; if (r !== rot) res.rotFixed.push(des + ':' + r); break; }
  }
  if (ok) res.placed++; else res.failed.push(des);
}

if (res.remaining || res.failed.length) { await eda.pcb_Document.save(); return JSON.stringify(res); }
// ---- outline (layer 11) and corner holes, only if missing (once every part is placed) ----
// EasyEDA 3.2.206 (2026-10-25): pcb_PrimitiveLine creates copper only and the v0.3 outline lines were gone after the
// update: the outline is one closed layer-11 polyline now (legacy layer-11 lines still count as an outline).
const legacyOutline = ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => String(l.getState_Layer()) === '11');
const polyOutline = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((p) => String(p.getState_Layer()) === '11');
if (!legacyOutline.length && !polyOutline.length) {
  await eda.pcb_PrimitivePolyline.create('', 11, eda.pcb_MathPolygon.createPolygon([0, 0, 'L', W, 0, W, H, 0, H, 0, 0]), 10);
  res.outline = 'created';
} else res.outline = 'exists';
const pads = (await eda.pcb_PrimitivePad.getAll()) || [];
const HOLES = [[170, 170], [W - 170, 170], [170, H - 170], [W - 170, H - 170]];
res.holes = 0;
for (const [hx, hy] of HOLES) {
  if (pads.some((p) => Math.abs(p.getState_X() - hx) < 1 && Math.abs(p.getState_Y() - hy) < 1)) continue;
  await eda.pcb_PrimitivePad.create(12, 'MH', hx, hy, 0, ['ELLIPSE', 236, 236], '', ['ROUND', 126], 0, 0, 0, true, 0);
  res.holes++;
}
await sleep(1000);
await eda.pcb_Document.save();
return JSON.stringify(res);
