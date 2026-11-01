// Station board v0.3 — step P4e: give every connected pin a short named wire stub (nets form by name) (WRITES).
// v0.5 (2026-10-25): C1, C2, D3, R1, TP1–TP6 removed (17b_remove_v05.js), J9.1 = +5V and D2.1 = VAMP_BAT (16_relabel_v05.js),
// J15 added (18_place_j15_v05.js places it; this script wires its 3 pins). Pins that already start a stub are skipped.
// Net map = WIRING-PLAN.md v0.5 §5 with the symbol pin numbers from the library files (09_symbol_fp_files.js, D-017):
//   1N4007 (D1, D2): 1 = K, 2 = A · LED C9900102778: 1 = +, 2 = − · electrolytics: 1 = + (silk "+" at pad 1)
//   v0.6 vertical XT (D-030): XT60UPB-M (J1) 1 = −, 2 = + · XT30UPB-M (J4) 1 = −, 2 = + · XT30UPB-F (J2, J3) 1 = +, 2 = − · tact SW1/SW2: pads 1 + 3 (D-016)
//   U2B = DFPlayer pins 16…9 (U2B.1 = BUSY, U2B.7 = GND).
// EXPECT below re-checks pin NAMES on the placed symbols before wiring a polarity part (aborts that part on a mismatch).
// Lockstep (playbook §7): settle until the wire count is stable, ≤ 60 stubs per run, then re-run until "remaining": 0.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\14_wire.js"
const NET = {};
const set = (des, map) => { NET[des] = Object.assign(NET[des] || {}, map); };
const two = (des, a, b) => set(des, { 1: a, 2: b });
const list = (arr) => Object.fromEntries(arr.map((n, i) => [String(i + 1), n]).filter(([, n]) => n));

// hub link (1:1 with the hub's station port) + LED stick
set('J1', list(['5V', 'GND', '3V3', 'DHT', 'MQ', 'PROX', 'DIN', 'DOUT']));
set('J2', list(['5V', 'GND', 'DIN', 'DOUT']));
// sensors on JST cables (v0.4): J3 DHT11 (3V3, GND, DOUT), J4 MQ-5 (5V, GND, AO, DO not connected) + AO divider
set('J3', list(['3V3', 'GND', 'DHT']));
set('J4', { 1: '5V', 2: 'GND', 3: 'MQ_AO' });
two('R2', 'MQ_AO', 'MQ'); two('R3', 'MQ', 'GND');
// proximity alarm: IR module (OUT GND VCC) -> R1 -> BC557 (emitter 5V) -> buzzer; D1 K on IR_OUT, A on PROX
set('U2', list(['IR_OUT', 'GND', '5V']));
two('R1', 'IR_OUT', 'Q_BASE');
set('Q1', { 1: 'BUZ_P', 2: 'Q_BASE', 3: '5V' });   // BC557B C2093 symbol: 1 = C, 2 = B, 3 = E (12_dump 2026-10-25)
set('D1', { 1: 'IR_OUT', 2: 'PROX' });             // 1 = K, 2 = A
set('BZ1', { 1: 'BUZ_P', 2: 'GND' });              // 1 = +, 2 = -

// polarity guard: expected pin names on the placed symbols
const EXPECT = { D1: { 1: 'K', 2: 'A' }, Q1: { 1: 'C', 2: 'B', 3: 'E' }, BZ1: { 1: '+' } };

// ---- draw (hub lessons: stub along the PIN ROTATION; cap a run at BATCH; never retry a refused create) ----
const PROJECT = 'station_pcb';
const L = 20;
const BATCH = 60;
// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: wires would not commit' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(300);
const DIR = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
// settle: wire creates commit late (> 2.5 s). Wait until the wire count is unchanged for 3 s (max 10 s), so a batch
// that is still committing can't be duplicated by this run.
{ let last = -1, since = Date.now(); const tS = Date.now();
  while (Date.now() - tS < 10000) { const n = ((await eda.sch_PrimitiveWire.getAllPrimitiveId()) || []).length;
    if (n !== last) { last = n; since = Date.now(); } else if (Date.now() - since >= 3000) break; await sleep(250); } }
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const started = new Set();
for (const w of wires) {
  const f = (w.getState_Line() || []).flat(3);
  for (let i = 0; i + 1 < f.length; i += 2) started.add(Math.round(f[i]) + ',' + Math.round(f[i + 1]));
}
const t0 = Date.now(); // time box starts after the settle step
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
let made = 0, skipped = 0, remaining = 0;
const errors = [], unknown = [], unmapped = [], polarity = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  const map = NET[des];
  if (!map) { unknown.push(des); continue; }
  const pins = (await c.getAllPins()) || [];
  if (EXPECT[des]) {
    const bad = pins.filter((p) => EXPECT[des][String(p.getState_PinNumber())] !== undefined &&
      EXPECT[des][String(p.getState_PinNumber())] !== String(p.getState_PinName()).replace('−', '-'));
    if (bad.length) { polarity.push(des + ': ' + bad.map((p) => p.getState_PinNumber() + '=' + p.getState_PinName()).join(',')); continue; }
  }
  const seen = new Set();
  for (const p of pins) {
    const num = String(p.getState_PinNumber());
    seen.add(num);
    const net = map[num];
    if (!net) continue;
    const x = Math.round(p.getState_X()), y = Math.round(p.getState_Y());
    if (started.has(x + ',' + y)) { skipped++; continue; }
    if (made >= BATCH || Date.now() - t0 > 12000) { remaining++; continue; } // settle (≤ 10 s) + creates + 2.5 s < 30 s bridge limit
    const [ux, uy] = DIR[((Math.round(p.getState_Rotation()) % 360) + 360) % 360] || [-1, 0];
    try {
      await eda.sch_PrimitiveWire.create([x, y, x + ux * L, y + uy * L], net);
      started.add(x + ',' + y);
      made++;
    } catch (e) {
      errors.push([des, num, String(e)]);
      await sleep(600);
    }
  }
  for (const k of Object.keys(map)) if (!seen.has(k)) unmapped.push(des + '.' + k);
}
await sleep(2500);
const committed = (await eda.sch_PrimitiveWire.getAllPrimitiveId()).length;
return JSON.stringify({ made, skipped, remaining, committed, unknown, unmapped, polarity, errors: errors.slice(0, 10) });
