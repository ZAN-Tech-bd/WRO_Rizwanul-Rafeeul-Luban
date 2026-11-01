// Robot carrier v0.5 — step P4e: give every connected pin a short named wire stub (nets form by name) (WRITES).
// v0.5 (2026-10-25): C1, C2, D3, R1, TP1–TP6 removed (17b_remove_v05.js), J9.1 = +5V and D2.1 = VAMP_BAT (16_relabel_v05.js),
// J15 added (18_place_j15_v05.js places it; this script wires its 3 pins). Pins that already start a stub are skipped.
// Net map = WIRING-PLAN.md v0.5 §5 with the symbol pin numbers from the library files (09_symbol_fp_files.js, D-017):
//   1N4007 (D1, D2): 1 = K, 2 = A · LED C9900102778: 1 = +, 2 = − · electrolytics: 1 = + (silk "+" at pad 1)
//   v0.6 vertical XT (D-030): XT60UPB-M (J1) 1 = −, 2 = + · XT30UPB-M (J4) 1 = −, 2 = + · XT30UPB-F (J2, J3) 1 = +, 2 = − · tact SW1/SW2: pads 1 + 3 (D-016)
//   U2B = DFPlayer pins 16…9 (U2B.1 = BUSY, U2B.7 = GND).
// EXPECT below re-checks pin NAMES on the placed symbols before wiring a polarity part (aborts that part on a mismatch).
// Lockstep (playbook §7): settle until the wire count is stable, ≤ 60 stubs per run, then re-run until "remaining": 0.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\14_wire.js"
const NET = {};
const set = (des, map) => { NET[des] = Object.assign(NET[des] || {}, map); };
const two = (des, a, b) => set(des, { 1: a, 2: b });
const list = (arr) => Object.fromEntries(arr.map((n, i) => [String(i + 1), n]).filter(([, n]) => n));

// power entry, switch, buck feeds, 5 V in
set('J1', { 1: 'GND', 2: 'BAT+' });
two('J5', 'BAT+', 'BAT_SW');
set('J2', { 1: 'BAT_SW', 2: 'GND' });
set('J3', { 1: 'BAT_SW', 2: 'GND' });
set('J4', { 1: 'GND', 2: '+5V' });
// amplifier rail VAMP + JP1
set('D1', { 2: 'BAT_SW', 1: 'AMP_D' });
set('D2', { 2: 'AMP_D', 1: 'VAMP_BAT' });   // v0.5: no R1 (D-023)
set('JP1', { 1: 'VAMP_BAT', 2: 'VAMP', 3: '+5V' });
two('C9', 'VAMP', 'GND'); two('C10', 'VAMP', 'GND');
// battery sense, LEDs (v0.5: no Mega diode D3, no test points)
two('R24', 'BAT_SW', 'VBAT_SENSE'); two('R25', 'VBAT_SENSE', 'GND'); two('C11', 'VBAT_SENSE', 'GND');
two('R26', 'BAT_SW', 'LED1_A'); two('LED1', 'LED1_A', 'GND');
two('R27', '+5V', 'LED2_A'); two('LED2', 'LED2_A', 'GND');
// DevKit sockets (pin 1 at the antenna end; spec §4)
set('U1A', list(['+3V3', '+3V3', null, 'ENA', 'IN1', 'IN2', 'IN3', 'IN4', 'ENB', 'DF_TO_S3_IO', 'CAM_TX_IO', 'DF_BUSY_IO',
  'EXP_IO3', null, 'RFID_RST', 'RFID_SS_IO', 'RFID_MOSI_IO', 'RFID_SCK_IO', 'RFID_MISO', 'S3_TO_DF_IO', '+5V', 'GND']));
set('U1B', list(['GND', null, null, 'VBAT_SENSE', 'OBST_IO', 'MEGA_RX_IO', 'MEGA_TX_IO', 'I2C_SDA_IO', 'I2C_SCL_IO', 'EXP_IO38',
  null, null, null, null, null, null, 'BTN2_IO', 'BTN1_IO', null, null, 'GND', 'GND']));
set('J14', list(['+3V3', 'GND', 'EXP_IO3', 'EXP_IO38']));
// motor (L298N)
set('J6', list(['+5V', 'GND', 'ENA', 'IN1', 'IN2', 'IN3', 'IN4', 'ENB']));
two('R2', 'ENA', 'GND'); two('R3', 'ENB', 'GND');
// RFID (RC522 on a cable)
set('J7', list(['+3V3', 'GND', 'RFID_SS', 'RFID_SCK', 'RFID_MOSI', 'RFID_MISO', 'RFID_RST']));
two('R4', 'RFID_SS_IO', 'RFID_SS'); two('R5', 'RFID_SCK_IO', 'RFID_SCK'); two('R6', 'RFID_MOSI_IO', 'RFID_MOSI');
two('C7', '+3V3', 'GND'); two('C8', '+3V3', 'GND');
// DFPlayer + LM386 sockets
set('U2A', { 1: '+5V', 2: 'S3_TO_DF', 3: 'DF_TO_S3', 4: 'DAC_R', 5: 'DAC_L', 7: 'GND' });
set('U2B', { 1: 'DF_BUSY', 7: 'GND' });
two('C3', '+5V', 'GND'); two('C4', '+5V', 'GND');
two('R7', 'S3_TO_DF_IO', 'S3_TO_DF'); two('R8', 'DF_TO_S3', 'DF_TO_S3_IO'); two('R9', 'DF_BUSY', 'DF_BUSY_IO');
set('U3', list(['VAMP', 'DAC_L', 'GND', 'GND']));
set('U4', list(['VAMP', 'DAC_R', 'GND', 'GND']));
// ESP32-CAM
set('J8', list(['+5V', 'GND', 'CAM_TX']));
two('C5', '+5V', 'GND'); two('C6', '+5V', 'GND');
two('R10', 'CAM_TX_IO', 'CAM_TX');
// Mega
set('J9', list(['+5V', 'GND', 'MEGA_TX', 'MEGA_RX']));   // v0.5: J9.1 = +5V direct (D-022)
two('R11', 'MEGA_TX_IO', 'MEGA_TX'); two('R12', 'MEGA_RX', 'MEGA_RX_IO'); two('R13', 'MEGA_RX_IO', 'GND');
// I2C (series protection R16/R17, D-010)
set('J10', list(['+3V3', 'GND', 'I2C_SCL', 'I2C_SDA']));
two('R14', '+3V3', 'I2C_SCL'); two('R15', '+3V3', 'I2C_SDA');
two('R16', 'I2C_SCL', 'I2C_SCL_IO'); two('R17', 'I2C_SDA', 'I2C_SDA_IO');
// obstacle sensors (wired-OR, D-025) + buttons (active LOW)
set('J11', list(['+3V3', 'GND', 'OBST']));
set('J15', list(['+3V3', 'GND', 'OBST']));
two('R18', '+3V3', 'OBST'); two('R19', 'OBST', 'OBST_IO'); two('C12', 'OBST_IO', 'GND');
set('J12', list(['BTN1', 'GND'])); set('SW1', { 1: 'BTN1', 3: 'GND' });
two('R20', '+3V3', 'BTN1'); two('R21', 'BTN1', 'BTN1_IO'); two('C13', 'BTN1_IO', 'GND');
set('J13', list(['BTN2', 'GND'])); set('SW2', { 1: 'BTN2', 3: 'GND' });
two('R22', '+3V3', 'BTN2'); two('R23', 'BTN2', 'BTN2_IO'); two('C14', 'BTN2_IO', 'GND');

// polarity guard: expected pin names on the placed symbols (D-017)
const EXPECT = {
  D1: { 1: 'K', 2: 'A' }, D2: { 1: 'K', 2: 'A' },
  LED1: { 1: '+', 2: '-' }, LED2: { 1: '+', 2: '-' }, J4: { 1: '-', 2: '+' }, J2: { 1: '+', 2: '-' }, J3: { 1: '+', 2: '-' },
  J1: { 1: '1', 2: '2' },   // v0.6 XT60UPB-M: pins named 1 / 2 only; 1 = − from its footprint (D-030)
};

// ---- draw (hub lessons: stub along the PIN ROTATION; cap a run at BATCH; never retry a refused create) ----
const PROJECT = 'WRO_Robot';
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
