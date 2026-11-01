// Step 10 — shift J1 clear of the outline, ROUTE all nets (Top layer), run DRC.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/24_route_drc.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const pcbTab = ((tree && tree.tabs) || []).find((t) => /^pcb/i.test(t.title));
if (!pcbTab) return JSON.stringify({ fail: 'no pcb tab' });
await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
await sleep(700);

// 1) nudge J1 right so pad1 (x=0) is off the outline
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids || []);
for (const c of comps) {
  if ((c.getState_Designator && c.getState_Designator()) === 'J1') {
    c.toSync().setState_X(150);
    log.j1Moved = true;
  }
}
await sleep(400);

// 2) traces (Top layer = 1, width 10). Pad coords AFTER the J1 shift (+50 x on J1 pads).
const T = 1, W = 10;
const traces = [
  // LED1_A: R1.2 -> LED1.A (straight, y=180)
  ['LED1_A', [329.7, 180, 579.5, 180]],
  // LED2_A: R2.2 -> LED2.A (straight, y=420)
  ['LED2_A', [329.7, 420, 579.5, 420]],
  // 5V: R1.1 -> R2.1 (vertical x=270.3)
  ['5V', [270.3, 180, 270.3, 420]],
  // 5V: J1.1 -> R1.1 (J1.1 now at (50,300))
  ['5V', [50, 300, 50, 180]],
  ['5V', [50, 180, 270.3, 180]],
  // GND: J1.3 (250,300) -> up to y=80 -> across -> down through LED1.K to LED2.K
  ['GND', [250, 300, 250, 80]],
  ['GND', [250, 80, 520.5, 80]],
  ['GND', [520.5, 80, 520.5, 420]],
];
log.traces = [];
for (const [net, [x1, y1, x2, y2]] of traces) {
  try {
    const r = await eda.pcb_PrimitiveLine.create(net, T, x1, y1, x2, y2, W);
    log.traces.push({ net, ok: !!r });
  } catch (e) {
    // fallback: layer as string
    try {
      await eda.pcb_PrimitiveLine.create(net, '1', x1, y1, x2, y2, W);
      log.traces.push({ net, ok: true, note: 'string layer' });
    } catch (e2) {
      log.traces.push({ net, ok: false, error: String(e2) });
    }
  }
}

await eda.pcb_Document.save();
await sleep(500);

// 3) DRC
try {
  log.drc = await eda.pcb_Drc.check();
} catch (e) {
  log.drcErr = String(e);
}

return JSON.stringify(log, null, 1);
