// Station board v0.3 — step P4b: place every part (spec §5) on station_pcb P1.Schematic1 with its designator (WRITES).
// Stage 1 of 2: parts go to a staging grid OUTSIDE the A4 frame (x >= 1300); 13_layout.js then packs them into function
// blocks using each symbol's real bbox. Idempotent + time-boxed: re-run until "remaining": 0.
// Gates: project station_pcb, schematic tab active, EasyEDA window visible (playbook §7: hidden windows freeze timers).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\11_place.js"
const PROJECT = 'station_pcb';
const L = '0819f05c4eef4c71ace90d822a990e87'; // LCSC system library (10_resolve.js)
const DEV = {   // 02_parts_verify.js (2026-10-25), LCSC system library
  XH8: '1966f7f1445e48c7984a4f0dbdc7b7e1', XH4: '7f13b11856f1454da74101a2bff96f83', FH3: 'b04adbc3c10841ada93585db547d01a9',
  FH4: 'b9529e96b6214087a1ee0b479fa2c97a', BC557: 'e8ea4a05ccdd4a65ba7b78eefb04723e', D5819: '86512b427b364f3c870adca12eb24b3e',
  R1K: 'bc411c54594f414b848db99ae40d2522', R68K: '39b59bb0c37940a7b6b11b89c95aeabd', R100K: 'e7dd8b41bd6b452c9064cd3645b972a3',
  BUZ: 'dbc34f25eeba4dc8a151f4e9c6741092', XH3: 'e3145416944046c8ae9e43248edd775d',
};
// [designator, device] in block order (spec v0.3 §5): A hub link + LED stick · B sensors · C proximity alarm
const P = [
  ['J1', 'XH8'], ['J2', 'XH4'],
  ['J3', 'XH3'], ['J4', 'XH4'], ['R2', 'R68K'], ['R3', 'R100K'],   // v0.4: DHT11 / MQ-5 on JST cables (U1 / U3 retired)
  ['U2', 'FH3'], ['R1', 'R1K'], ['Q1', 'BC557'], ['D1', 'D5819'], ['BZ1', 'BUZ'],
];
const DNP = new Set([]); // pads only: AddIntoBom = false

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
