// Hub v0.4b PCB — top-silkscreen labels + outlines for hand assembly and wiring (layer 3), for the 2026-10-26 floor plan
// (4600 x 3900 mil, layout_plan.py). Every label is placed from the REAL pad positions read from the board, so it
// follows the parts; the DevKit / SD outlines come from U1A / U1B / U6 pads. Owns ALL free layer-3 texts and lines:
// deletes them and redraws (the hub has no hand-made silk). Run after 21b_place_pcb.js, then 25_designators.js,
// 24_check_silk.js, 26_check_designators.js. Gates: WRO_HUB, PCB tab, visible.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/23_silkscreen.js"
const SILK = 3, CENTER = 5, LEFT_MID = 2;
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== 'WRO_HUB') return JSON.stringify({ abort: 'open project is not WRO_HUB' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }

const comps = await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId());
const pad = {}, pos = {};
for (const c of comps) {
  const d = c.getState_Designator();
  pad[d] = {}; pos[d] = [c.getState_X(), c.getState_Y()];
  for (const p of (await c.getAllPins()) || []) pad[d][String(p.getState_PadNumber())] = [p.getState_X(), p.getState_Y()];
}
const T = []; // [x, y, text, size, rotation, align]
const L = []; // [x1, y1, x2, y2]
const t = (x, y, s, size = 40, rot = 0, align = CENTER) => T.push([Math.round(x), Math.round(y), s, size, rot, align]);
const mid = (d, a, b) => [(pad[d][a][0] + pad[d][b][0]) / 2, (pad[d][a][1] + pad[d][b][1]) / 2];

// ---- DevKit outline (27.94 x 57.15 mm, top flush with the board edge) + USB lane ----
const xa = pad.U1A['1'][0] - 56, xb = pad.U1B['1'][0] + 56, ytop = pad.U1A['1'][1] + 75, ybot = ytop - 2250;
L.push([xa, ybot, xb, ybot], [xa, ybot, xa, ytop - 20], [xb, ybot, xb, ytop - 20]);
const dkx = (xa + xb) / 2;
t(dkx, 3810, 'ANTENNA ^ over edge', 32);
t(dkx, 2700, 'ESP32-S3 DEVKIT  (YD N16R8)', 50, 90);
t(pad.U1A['1'][0] - 70, pad.U1A['1'][1], '1', 32); t(pad.U1B['1'][0] + 70, pad.U1B['1'][1], '1', 32);
t(dkx, ybot + 50, 'USB v', 32);
t(dkx, 880, 'USB LANE: low parts only', 30);
// ---- title (empty bottom of the USB lane) ----
t(dkx, 340, 'WRO MUSEUM HUB v0.4b', 50);
t(dkx, 260, 'ESP32-S3 N16R8 carrier  2026-10', 32);

// ---- power column (vertical XT: cables from above); names beside the column, polarity right of each pad ----
const side = (d, s) => t(pad.J3['1'][0] + 220, (pad[d]['1'][1] + pad[d]['2'][1]) / 2, s, 36, 90);
side('J1', 'BATTERY 12V'); side('J2', 'DC 12V'); side('J4', 'TO BUCK 12V');
const pm = (d, plus, minus) => { t(pad[d][plus][0] + 150, pad[d][plus][1], '+', 44); t(pad[d][minus][0] + 150, pad[d][minus][1], '-', 44); };
pm('J1', '2', '1'); pm('J2', '2', '1'); pm('J4', '1', '2');   // XT60UPB-M 1 = -, 2 = +; XT60UPB-F 1 = +, 2 = -
t(pad.J3['1'][0] + 220, mid('J3', '1', '6')[1], 'SWITCH J3', 36, 90);
t(pad.J3['1'][0] + 120, pad.J3['1'][1], '1', 32);
t(260, 3560, 'J3 SWITCH: 1 BAT+', 26); t(260, 3500, '2 V12 3 DC+ 4 BAT-', 26); t(260, 3440, '5 GND 6 DC-', 26);
// J5 XT30UPB-M (1 = -, 2 = +), 5 V back from the buck
t(mid('J5', '1', '2')[0], pad.J5['1'][1] - 155, '-  5V FROM BUCK  +', 28);   // "-" under pad 1, "+" under pad 2

// ---- LEDs: meaning above, designators below (25_designators) ----
t(pos.LED1[0], pos.LED1[1] + 162, 'BAT', 28); t(pos.LED2[0], pos.LED2[1] + 162, 'DC', 28); t(pos.LED3[0], pos.LED3[1] + 144, '5V', 28);

// ---- E-stop, I2C (pin names in one line under each port) ----
t(pos.J20[0], pos.J20[1] - 178, 'E-STOP: 3V3 GND SIG', 24);
t(pos.J18[0], pos.J18[1] - 150, 'I2C 3.3V: 3V3 GND SCL SDA', 24);
t(pad.Q1['3'][0] + 125, pad.Q1['2'][1], 'EBC', 24);

// ---- servo ports 2 x 2 (pin 1 = 5V on the left) ----
// designators J21-J24 stand left of each pin 1 (25_designators); one legend between the rows
t((pos.J21[0] + pos.J22[0]) / 2, (pos.J21[1] + pos.J23[1]) / 2 - 19, 'SERVO J21-J24: 5V GND SIG', 24);

// ---- spare 5 V outputs J25 / J26 (pin 1 = 5V on the right) ----
for (const d of ['J25', 'J26']) { t(pad[d]['1'][0], pad[d]['1'][1] + 150, '5V', 24); t(pad[d]['2'][0], pad[d]['2'][1] + 150, 'GND', 24); }
t((pos.J25[0] + pos.J26[0]) / 2, pos.J25[1] - 158, 'SPARE 5V OUT', 24);

// ---- SD module (46 x 24 mm) lying flat from its header U6 towards the bottom edge ----
const sx0 = pad.U6['1'][0] - 222, sx1 = pad.U6['6'][0] + 222, sy1 = pad.U6['1'][1] + 60, sy0 = 20;
L.push([sx0, sy0, sx1, sy0], [sx1, sy0, sx1, sy1], [sx1, sy1, sx0, sy1], [sx0, sy1, sx0, sy0]);
t((sx0 + sx1) / 2, 110, 'SD MODULE lies flat, card slot over edge v', 24);
['CS', 'SCK', 'MOSI', 'MISO', 'VCC', 'GND'].forEach((n, i) => t(pad.U6[String(i + 1)][0], pad.U6['1'][1] + 120, n, 26));

// ---- stations ----
const PINS = ['5V', 'GND', '3V3', 'DHT', 'MQ', 'PRX', 'DI', 'DO'];
for (let k = 1; k <= 8; k++) {
  const d = 'J' + (9 + k), p = pad[d];
  const cx = (p['1'][0] + p['8'][0]) / 2, cy = p['1'][1];
  t(cx - 410, cy + 175, `S${k}  Z${Math.ceil(k / 2)}`, 40, 0, LEFT_MID);
  PINS.forEach((s, i) => t(p[String(i + 1)][0], cy - 133, k === 8 && (i === 3 || i === 4) ? 'nc' : s, 24));
}
const sbx = (pad.J10['8'][0] + pad.J17['1'][0]) / 2;
t(sbx, 3700, 'LED chain: S1>S2>S3>S4 > S5>S6>S7>S8', 30);
t(sbx, 110, 'SBx bypass: bridge ONLY if that port is empty', 26);

// ---- redraw: every free layer-3 text / line belongs to this script ----
const oldT = ((await eda.pcb_PrimitiveString.getAll()) || []).filter((s) => String(s.getState_Layer()) === String(SILK));
if (oldT.length) await eda.pcb_PrimitiveString.delete(oldT.map((s) => s.getState_PrimitiveId()));
// EasyEDA 3.2.206: silk lines are 2-point polylines (pcb_PrimitiveLine creates copper only)
const oldLegacy = ((await eda.pcb_PrimitiveLine.getAll()) || []).filter((l) => String(l.getState_Layer()) === String(SILK));
if (oldLegacy.length) await eda.pcb_PrimitiveLine.delete(oldLegacy.map((l) => l.getState_PrimitiveId()));
const oldL = ((await eda.pcb_PrimitivePolyline.getAll()) || []).filter((l) => String(l.getState_Layer()) === String(SILK));
if (oldL.length) await eda.pcb_PrimitivePolyline.delete(oldL.map((l) => l.getState_PrimitiveId()));
let made = 0;
for (const [x, y, s, size, rot, align] of T) {
  await eda.pcb_PrimitiveString.create(SILK, x, y, s, 'default', size, Math.max(5, Math.round(size / 7)), align, rot, false, 0, false, false);
  made++;
}
for (const [x1, y1, x2, y2] of L) await eda.pcb_PrimitivePolyline.create('', SILK, eda.pcb_MathPolygon.createPolygon([x1, y1, 'L', x2, y2]), 6);
await eda.pcb_Document.save();
return JSON.stringify({ texts: made, lines: L.length, removedTexts: oldT.length, removedLines: oldL.length + oldLegacy.length });
