// Hub v0.4 — step 3d: give every connected pin a short named wire stub (nets form by name).
// v0.4 (2026-10-25): no R14 / R21–R27 (retired), J25 / J26 spare 5 V ports (1 = +5V, 2 = GND). Gates: WRO_HUB + visible.
// Net map = WIRING-PLAN.md §5 with the real symbol pin numbers read by 08_dump_pins.js
// (vertical XT, 2026-10-26: XT60UPB-M 1 = -, 2 = +; XT60UPB-F 1 = +, 2 = -; XT30UPB-M 1 = -, 2 = +; Q1: 1 E, 2 B, 3 C;
//  D1: 1 = cathode, 2 = anode; LEDs/BZ1: 1 = +, 2 = -; C2: 1 = + (verify square pad in PCB)).
// Idempotent + time-boxed: skips pins that already start a wire; re-run until "remaining": 0.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/09_wire_nets.js"
const _proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!_proj || (_proj.friendlyName || _proj.name) !== 'WRO_HUB') return JSON.stringify({ abort: 'open project is not WRO_HUB' });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const NET = {};
const set = (des, map) => { NET[des] = Object.assign(NET[des] || {}, map); };
const list = (arr) => Object.fromEntries(arr.map((n, i) => [String(i + 1), n]).filter(([, n]) => n));

// power
set('J1', { 2: 'BAT+', 1: 'BAT-' });
set('J2', { 2: 'DC+', 1: 'DC-' });
set('J3', list(['BAT+', 'V12', 'DC+', 'BAT-', 'GND', 'DC-']));
set('J4', { 1: 'V12', 2: 'GND' });   // D-031: XT60UPB-F vertical, 1 = +, 2 = -
set('J5', { 2: '+5V', 1: 'GND' });
set('R1', { 1: 'BAT+', 2: 'LEDR_A' }); set('LED1', { 1: 'LEDR_A', 2: 'GND' });
set('R2', { 1: 'DC+', 2: 'LEDB_A' }); set('LED2', { 1: 'LEDB_A', 2: 'GND' });
set('R3', { 1: '+5V', 2: 'LEDG_A' }); set('LED3', { 1: 'LEDG_A', 2: 'GND' });
// DevKit headers
set('U1A', list(['+3V3', '+3V3', null, 'MQ_S1_ADC', 'MQ_S2_ADC', 'MQ_S3_ADC', 'MQ_S4_ADC', 'DHT_S1_IO',
  'DHT_S2_IO', 'DHT_S3_IO', 'DHT_S4_IO', 'I2C_SDA', 'PROX_Z3_IO', 'SERVO_IN', 'I2C_SCL', 'MQ_S7_ADC',
  'SD_MOSI', 'SD_SCK', 'SD_MISO', 'SD_CS', '+5V', 'GND']));
set('U1B', list(['GND', null, null, 'MQ_S5_ADC', 'MQ_S6_ADC', 'PROX_Z2_IO', 'PROX_Z1_IO', 'DHT_S7_IO',
  'DHT_S6_IO', 'DHT_S5_IO', null, null, null, null, 'LED_DIN', 'PROX_Z4_IO', 'BUZ_IO', 'ESTOP_IO',
  null, null, 'GND', 'GND']));
set('U6', list(['SD_CS', 'SD_SCK', 'SD_MOSI', 'SD_MISO', '+5V', 'GND']));
// 74HCT125
set('U2', list(['GND', 'LED_DIN', 'LED_BUF', 'GND', 'SERVO_IN', 'SERVO_BUF', 'GND', null, 'GND', '+5V',
  null, 'GND', '+5V', '+5V']));
set('C1', { 1: '+5V', 2: 'GND' });
set('R4', { 1: 'LED_DIN', 2: 'GND' });
set('R5', { 1: 'SERVO_IN', 2: 'GND' });
set('R6', { 1: 'LED_BUF', 2: 'S1_DIN' });
for (let s = 1; s <= 4; s++) {
  set('R' + (6 + s), { 1: 'SERVO_BUF', 2: 'SERVO' + s + '_SIG' });
  set('J' + (20 + s), { 1: '+5V', 2: 'GND', 3: 'SERVO' + s + '_SIG' });
}
set('C2', { 1: '+5V', 2: 'GND' });
// buzzer
set('R11', { 1: 'BUZ_IO', 2: 'BUZ_B' });
set('R12', { 1: 'BUZ_B', 2: 'GND' });
set('Q1', { 1: 'GND', 2: 'BUZ_B', 3: 'BUZ_N' });
set('BZ1', { 1: '+5V', 2: 'BUZ_N' });
set('D1', { 2: 'BUZ_N', 1: '+5V' });
// emergency button, I2C
set('J20', { 1: '+3V3', 2: 'GND', 3: 'ESTOP' });
set('R13', { 1: 'ESTOP', 2: 'ESTOP_IO' });
set('J18', { 1: '+3V3', 2: 'GND', 3: 'I2C_SCL', 4: 'I2C_SDA' });
set('R15', { 1: '+3V3', 2: 'I2C_SCL' });
set('R16', { 1: '+3V3', 2: 'I2C_SDA' });
// spare 5 V outputs (v0.4)
set('J25', { 1: '+5V', 2: 'GND' });
set('J26', { 1: '+5V', 2: 'GND' });
// stations
for (let k = 1; k <= 8; k++) {
  const z = 'PROX_Z' + Math.ceil(k / 2);
  const j = 'J' + (9 + k);
  if (k <= 7) {
    set(j, list(['+5V', 'GND', '+3V3', 'DHT_S' + k, 'MQ_S' + k + '_ADC', z, 'S' + k + '_DIN', 'S' + k + '_DOUT']));
    set('R' + (30 + k), { 1: 'DHT_S' + k, 2: 'DHT_S' + k + '_IO' });
    set('C' + (2 + k), { 1: 'MQ_S' + k + '_ADC', 2: 'GND' });
    set('R' + (60 + k), { 1: 'S' + k + '_DOUT', 2: 'S' + (k + 1) + '_DIN' });
    set('SB' + k, { 1: 'S' + k + '_DIN', 2: 'S' + k + '_DOUT' });
  } else {
    set(j, { 1: '+5V', 2: 'GND', 3: '+3V3', 6: z, 7: 'S8_DIN' }); // pin 8 = chain end, No Connect on the hub
  }
}
for (let n = 1; n <= 4; n++) {
  set('R' + (40 + n), { 1: '+3V3', 2: 'PROX_Z' + n });
  set('R' + (50 + n), { 1: 'PROX_Z' + n, 2: 'PROX_Z' + n + '_IO' });
}

// ---- draw ----
// Lessons (2026-10-12): (1) the stub must follow the PIN ROTATION (0 = +x, 90 = +y, 180 = -x, 270 = -y);
// "away from the part centre" sent stubs along long header columns over neighbouring pins -> shorted nets
// -> EasyEDA then refused every create ("create failed!"). (2) create() returns before the canvas commits,
// so cap each run at BATCH wires and let the editor settle; a stored line may start at either end.
const t0 = Date.now();
const L = 30;
const BATCH = 60;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const DIR = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const started = new Set();
for (const w of wires) {
  const f = (w.getState_Line() || []).flat(3);
  for (let i = 0; i + 1 < f.length; i += 2) started.add(Math.round(f[i]) + ',' + Math.round(f[i + 1]));
}
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
let made = 0, skipped = 0, remaining = 0;
const errors = [];
const unknown = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  const map = NET[des];
  if (!map) { unknown.push(des); continue; }
  for (const p of (await c.getAllPins()) || []) {
    const net = map[String(p.getState_PinNumber())];
    if (!net) continue;
    const x = Math.round(p.getState_X()), y = Math.round(p.getState_Y());
    if (started.has(x + ',' + y)) { skipped++; continue; }
    if (made >= BATCH || Date.now() - t0 > 18000) { remaining++; continue; }
    const [ux, uy] = DIR[((Math.round(p.getState_Rotation()) % 360) + 360) % 360] || [-1, 0];
    const end = [x + ux * L, y + uy * L];
    // NO immediate retry: a create that throws while the editor is busy can still commit later, and a
    // retry then stacks a second identical wire (EasyEDA merges it -> one wire with its name twice ->
    // "multiple net names" ERC warning). A pin that really failed is simply picked up by the next run.
    try {
      await eda.sch_PrimitiveWire.create([x, y, end[0], end[1]], net);
      started.add(x + ',' + y);
      made++;
    } catch (e) {
      errors.push([des, p.getState_PinNumber(), String(e)]);
      await sleep(600);
    }
  }
}
await sleep(2500); // let the canvas commit before the next run reads it (save once at the end, in 10_check.js)
const committed = (await eda.sch_PrimitiveWire.getAllPrimitiveId()).length;
return JSON.stringify({ made, skipped, remaining, committed, unknown, errors: errors.slice(0, 10) });
