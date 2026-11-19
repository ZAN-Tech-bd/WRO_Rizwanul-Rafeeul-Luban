// Hub v0.4 — step 3b: place every part on WRO_HUB P1.Schematic1 with its final designator.
// v0.4 (2026-10-25): R14 and the DHT pull-ups R21–R27 retired (14_remove_v04.js); J25 / J26 spare 5 V ports added
// (15_add_spare5v_v04.js places them on the live, hand-arranged sheet; the coordinates below are only for a fresh sheet).
// Idempotent + time-boxed: skips designators already on the sheet; re-run until "remaining": 0.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/07_place.js"
const L = '0819f05c4eef4c71ace90d822a990e87'; // LCSC system library (from 06_resolve_devices.js)
const DEV = {
  XT60M: '057d6ce6748b425a882be950950c4f86', XT60F: '3a81952726df47549fbe3096f5a118ef',
  XT30M: 'cd3fca17cad04ef396824a6d5e19ffc5', VH6: 'ca315fd6a3f5437fae45de58081eefc2',
  XH8: '1966f7f1445e48c7984a4f0dbdc7b7e1', XH4: '7f13b11856f1454da74101a2bff96f83',
  XH3: 'e3145416944046c8ae9e43248edd775d', FH22: 'cc13e4dacba3435c927e1b1d310dffcf',
  FH6: 'f066af35d23f4280b9735a273f24677f', HCT125: 'f90e9212e6fa417bb25d71c591d36dc4',
  NPN: 'daa72fbe9b624d66afdbb9639f1de9db', D4148: '1cd391b03f36496c9af2eda9f38f1fc3',
  R10K: '9b81c60c26fb4475a9110def3339918f', R1K: 'bc411c54594f414b848db99ae40d2522',
  R220: 'f2ea6101fdef4370b8a0972ac25f68b7', R330: '686f7cc1a4ff46dd875bd3f3fb8ded71',
  R2K2: 'd0f2e43bbc8f4ac09548117becf10484', R4K7: '8bf20b3dd8c243fbbeff5d8a72ea5d63',
  C100N: '86d898f340ee4ce9a13303510059829f', C470U: '0f790b9449dc4eb5bbe42d0a21551819',
  BUZ: 'dbc34f25eeba4dc8a151f4e9c6741092', LED5: '724ea17ba9284320abeefad6ea567339',
  SB: '32d09cae88944ea8bd9e366e90edf831', XH2: '3b9af90046a04769ada1b68e95fdf6de',
};

// [designator, device, x, y]  — units: 10 mil
const P = [
  // power
  ['J1', 'XT60M', 200, 2400], ['J2', 'XT60M', 200, 2200], ['J3', 'VH6', 200, 1950],
  ['J4', 'XT60F', 200, 1650], ['J5', 'XT30M', 200, 1450],
  ['R1', 'R2K2', 500, 2400], ['LED1', 'LED5', 700, 2400],
  ['R2', 'R2K2', 500, 2250], ['LED2', 'LED5', 700, 2250],
  ['R3', 'R1K', 500, 2100], ['LED3', 'LED5', 700, 2100],
  // MCU + SD
  ['U1A', 'FH22', 1100, 1900], ['U1B', 'FH22', 1500, 1900], ['U6', 'FH6', 1300, 1300],
  // 74HCT125, LED out, servos
  ['U2', 'HCT125', 2150, 2300], ['C1', 'C100N', 2150, 2500],
  ['R4', 'R10K', 1900, 2400], ['R5', 'R10K', 1900, 2250], ['R6', 'R330', 2450, 2400],
  ['R7', 'R220', 2450, 2250], ['R8', 'R220', 2450, 2150], ['R9', 'R220', 2450, 2050], ['R10', 'R220', 2450, 1950],
  ['J21', 'XH3', 2700, 2300], ['J22', 'XH3', 2700, 2150], ['J23', 'XH3', 2700, 2000], ['J24', 'XH3', 2700, 1850],
  ['C2', 'C470U', 2700, 1700],
  // buzzer
  ['R11', 'R1K', 1900, 1500], ['R12', 'R10K', 1900, 1400], ['Q1', 'NPN', 2150, 1450],
  ['BZ1', 'BUZ', 2400, 1550], ['D1', 'D4148', 2400, 1400],
  // emergency button, I2C
  ['R13', 'R1K', 2400, 1200], ['J20', 'XH3', 2700, 1150],
  ['R15', 'R4K7', 2400, 950], ['R16', 'R4K7', 2400, 850], ['J18', 'XH4', 2700, 900],
  // spare 5 V outputs (v0.4)
  ['J25', 'XH2', 200, 1250], ['J26', 'XH2', 200, 1100],
];
// stations: row k = 1..8
for (let k = 1; k <= 8; k++) {
  const y = 2450 - (k - 1) * 300;
  P.push(['J' + (9 + k), 'XH8', 3050, y]);
  if (k <= 7) {
    P.push(['R' + (30 + k), 'R1K', 3550, y + 80]); // DHT series
    P.push(['C' + (2 + k), 'C100N', 3350, y]); // MQ filter
    P.push(['R' + (60 + k), 'R220', 3750, y - 40]); // LED hop Sk_DOUT -> S(k+1)_DIN
    P.push(['SB' + k, 'SB', 3750, y + 40]); // LED bypass
  }
  if (k % 2 === 1) {
    const n = (k + 1) / 2; // proximity zone
    P.push(['R' + (40 + n), 'R10K', 3350, y - 80]); // zone pull-up
    P.push(['R' + (50 + n), 'R1K', 3550, y - 80]); // zone series
  }
}

const t0 = Date.now();
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (tab) { await eda.dmt_EditorControl.activateDocument(tab.tabId); await sleep(300); }

const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = ids && ids.length ? await eda.sch_PrimitiveComponent.get(ids) : [];
const have = new Set((Array.isArray(comps) ? comps : [comps]).map((c) => c && c.getState_Designator && c.getState_Designator()));

const placed = [];
const errors = [];
let skipped = 0;
for (const [des, key, x, y] of P) {
  if (have.has(des)) { skipped++; continue; }
  if (Date.now() - t0 > 21000) break;
  try {
    const comp = await eda.sch_PrimitiveComponent.create({ libraryUuid: L, uuid: DEV[key] }, x, y);
    const a = comp.toAsync();
    a.setState_Designator(des);
    await a.done();
    placed.push(des);
  } catch (e) {
    errors.push([des, String(e)]);
  }
}
await eda.sch_Document.save();
return JSON.stringify({ total: P.length, placedNow: placed.length, skipped, remaining: P.length - skipped - placed.length, errors });
