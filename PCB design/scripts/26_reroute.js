// Step 12 — REWORK routing (user review found shorts: LED_A traces crossed GND K-pads).
// 1) delete old GND/LED traces (Top)  2) redraw with jogs  3) GND -> Bottom layer + 2 vias
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/26_reroute.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const pcbTab = ((tree && tree.tabs) || []).find((t) => /^pcb/i.test(t.title));
if (!pcbTab) return JSON.stringify({ fail: 'no pcb tab' });
await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
await sleep(700);

// 1) delete bad traces
try {
  const lids = await eda.pcb_PrimitiveLine.getAllPrimitiveId();
  const lines = await eda.pcb_PrimitiveLine.get(lids || []);
  const bad = [];
  for (const l of lines) {
    const net = typeof l.getState_Net === 'function' ? l.getState_Net() : '';
    if (net === 'GND' || net === 'LED1_A' || net === 'LED2_A') bad.push(l.getState_PrimitiveId());
  }
  if (bad.length && typeof eda.pcb_PrimitiveLine.delete === 'function') {
    await eda.pcb_PrimitiveLine.delete(bad);
    log.deleted = bad.length;
  } else {
    log.deleteNote = bad.length + ' found; delete method: ' + typeof eda.pcb_PrimitiveLine.delete;
    if (bad.length) return JSON.stringify(log, null, 1); // cannot proceed safely
  }
} catch (e) {
  log.delErr = String(e);
  return JSON.stringify(log, null, 1);
}

// 2) redraw — Top (1): 5V + LED nets with clean jogs; Bottom (2): GND
const segs = [
  // 5V (Top)
  ['5V', 1, 50, 300, 50, 230],
  ['5V', 1, 50, 230, 270.3, 230],
  ['5V', 1, 270.3, 230, 270.3, 180], // up to R1.1
  ['5V', 1, 270.3, 230, 270.3, 420], // down to R2.1
  // LED1_A (Top, jog over the top — clears the K pad)
  ['LED1_A', 1, 329.7, 180, 329.7, 110],
  ['LED1_A', 1, 329.7, 110, 660, 110],
  ['LED1_A', 1, 660, 110, 660, 180],
  ['LED1_A', 1, 660, 180, 579.5, 180],
  // LED2_A (Top, jog under the bottom — clears the K pad)
  ['LED2_A', 1, 329.7, 420, 329.7, 500],
  ['LED2_A', 1, 329.7, 500, 660, 500],
  ['LED2_A', 1, 660, 500, 660, 420],
  ['LED2_A', 1, 660, 420, 579.5, 420],
  // GND (Bottom layer 2): J1.3 is TH (reaches bottom); LEDs get vias
  ['GND', 2, 250, 300, 250, 460],
  ['GND', 2, 250, 460, 520.5, 460],
  ['GND', 2, 520.5, 460, 520.5, 180],
];
log.traces = [];
for (const [net, layer, x1, y1, x2, y2] of segs) {
  try {
    const r = await eda.pcb_PrimitiveLine.create(net, layer, x1, y1, x2, y2, 10);
    log.traces.push({ net, layer, ok: !!r });
  } catch (e) {
    log.traces.push({ net, layer, ok: false, error: String(e) });
  }
}

// 3) vias at the two LED K pads (top pad -> bottom trace)
log.vias = [];
for (const [x, y] of [[520.5, 180], [520.5, 420]]) {
  try {
    const v = await eda.pcb_PrimitiveVia.create('GND', x, y, 20, 40);
    log.vias.push({ x, y, ok: !!v });
  } catch (e) {
    log.vias.push({ x, y, ok: false, error: String(e) });
  }
}

await eda.pcb_Document.save();
await sleep(500);

// 4) strict DRC this time
try {
  log.drc = await eda.pcb_Drc.check(true);
} catch (e) {
  log.drcErr = String(e);
}

return JSON.stringify(log, null, 1);
