// Robot carrier v0.5 — step P4b: place every part (spec §5) on WRO_Robot P1.Schematic1 with its designator (WRITES).
// Stage 1 of 2: parts go to a staging grid OUTSIDE the A4 frame (x >= 1300); 13_layout.js then packs them into function
// blocks using each symbol's real bbox. Idempotent + time-boxed: re-run until "remaining": 0.
// Gates: project WRO_Robot, schematic tab active, EasyEDA window visible (playbook §7: hidden windows freeze timers).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\11_place.js"
const PROJECT = 'WRO_Robot';
const L = '0819f05c4eef4c71ace90d822a990e87'; // LCSC system library (10_resolve.js)
const DEV = {
  XT60M: '057d6ce6748b425a882be950950c4f86', XT30M: 'cd3fca17cad04ef396824a6d5e19ffc5',
  XT30F: '056de028c12d4b6dae321073898b67eb', VH2: '0ab8b24e310146e0a7bc87fb3e34ce52',
  XH8: '1966f7f1445e48c7984a4f0dbdc7b7e1', XH7: '7207ca961e09413f9876c59c26a80537',
  XH4: '7f13b11856f1454da74101a2bff96f83', XH3: 'e3145416944046c8ae9e43248edd775d',
  XH2: '3b9af90046a04769ada1b68e95fdf6de', FH22: 'cc13e4dacba3435c927e1b1d310dffcf',
  FH8: '8cc2e111422a42bd830a20f4e9ac086d', FH4: 'b9529e96b6214087a1ee0b479fa2c97a',
  HDR1: '41054977296a44b6861f2c322c6316d3', HDR3: '3d472c23b2204deab7cf7a842d521d9c',
  HDR4: '3fc6f660865344b4b66bc756a1282e83', D4007: 'a93a1d7520f04684aeac8b4b9a572e78',
  D5819: '86512b427b364f3c870adca12eb24b3e', TACT: '5806951b7faa4afa9c0715b8a8f2cb38',
  R1R1: '914b798d8d6746dfb433f0e5c839c19e', R47: '1bc39774a62c462fabe38457ea1b335c',
  R1K: 'bc411c54594f414b848db99ae40d2522', R2K2: 'd0f2e43bbc8f4ac09548117becf10484',
  R4K7: '8bf20b3dd8c243fbbeff5d8a72ea5d63', R10K: '9b81c60c26fb4475a9110def3339918f',
  R20K: '5c094bccf9f1454d946838d0c9b208b6', R100K: 'e7dd8b41bd6b452c9064cd3645b972a3',
  C100N: '86d898f340ee4ce9a13303510059829f', C470U: '0f790b9449dc4eb5bbe42d0a21551819',
  C1000U16: '97f1c80616e4430e8aeb2754236a8696', C1000U35: 'c95b6241605f406fbeed647977b5b130',
  C10U: '0a889ed7a0ef48c9bb122a351e12ab4e', LED5: '724ea17ba9284320abeefad6ea567339',
};
// v0.5 (2026-10-25): C1, C2, D3, R1, TP1–TP6 retired (spec v0.5 header); J15 added (on a live sheet 18_place_j15_v05.js
// places it into block G, because re-running 13_layout.js would move wired parts away from their stubs).
// [designator, device] in block order (spec v0.5 §5): A power/switch/LEDs/VBAT · B DevKit sockets/EXP · C motor ·
// D RFID · E audio · F CAM + Mega · G inputs + I2C
const P = [
  ['J1', 'XT60M'], ['J5', 'VH2'], ['J2', 'XT30F'], ['J3', 'XT30F'], ['J4', 'XT30M'],
  ['R26', 'R2K2'], ['LED1', 'LED5'], ['R27', 'R1K'], ['LED2', 'LED5'], ['R24', 'R100K'], ['R25', 'R20K'], ['C11', 'C100N'],
  ['U1A', 'FH22'], ['U1B', 'FH22'], ['J14', 'HDR4'],
  ['J6', 'XH8'], ['R2', 'R10K'], ['R3', 'R10K'],
  ['J7', 'XH7'], ['R4', 'R47'], ['R5', 'R47'], ['R6', 'R47'], ['C7', 'C10U'], ['C8', 'C100N'],
  ['U2A', 'FH8'], ['U2B', 'FH8'], ['C3', 'C470U'], ['C4', 'C100N'], ['R7', 'R1K'], ['R8', 'R1K'], ['R9', 'R1K'],
  ['U3', 'FH4'], ['U4', 'FH4'], ['D1', 'D4007'], ['D2', 'D4007'], ['JP1', 'HDR3'], ['C9', 'C1000U35'],
  ['C10', 'C100N'],
  ['J8', 'XH3'], ['C5', 'C470U'], ['C6', 'C100N'], ['R10', 'R1K'], ['J9', 'XH4'], ['R11', 'R1K'],
  ['R12', 'R10K'], ['R13', 'R20K'],
  ['J10', 'XH4'], ['R14', 'R4K7'], ['R15', 'R4K7'], ['R16', 'R1K'], ['R17', 'R1K'], ['J11', 'XH3'], ['J15', 'XH3'], ['R18', 'R10K'],
  ['R19', 'R1K'], ['C12', 'C100N'], ['J12', 'XH2'], ['SW1', 'TACT'], ['R20', 'R10K'], ['R21', 'R1K'], ['C13', 'C100N'],
  ['J13', 'XH2'], ['SW2', 'TACT'], ['R22', 'R10K'], ['R23', 'R1K'], ['C14', 'C100N'],
];
const DNP = new Set(['J14']); // pads only: AddIntoBom = false

// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(300);

const t0 = Date.now();
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = ids && ids.length ? await eda.sch_PrimitiveComponent.get(ids) : [];
const have = new Set((Array.isArray(comps) ? comps : [comps]).map((c) => c && c.getState_Designator && c.getState_Designator()));

const placed = [], errors = [];
let skipped = 0;
for (let i = 0; i < P.length; i++) {
  const [des, key] = P[i];
  if (have.has(des)) { skipped++; continue; }
  if (Date.now() - t0 > 18000) break;
  const x = 1300 + (i % 10) * 160, y = 100 + Math.floor(i / 10) * 160; // staging grid, outside the A4 frame
  try {
    const comp = await eda.sch_PrimitiveComponent.create({ libraryUuid: L, uuid: DEV[key] }, x, y);
    const a = comp.toAsync();
    a.setState_Designator(des);
    if (DNP.has(des)) a.setState_AddIntoBom(false);
    await a.done();
    placed.push(des);
  } catch (e) {
    errors.push([des, String(e)]);
  }
}
await eda.sch_Document.save();
return JSON.stringify({ total: P.length, placedNow: placed.length, skipped, remaining: P.length - skipped - placed.length, errors });
