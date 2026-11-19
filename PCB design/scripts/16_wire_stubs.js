// Step 4b — FIX: nets form via WIRES, not bare labels. Draw a short stub wire from each
// pin coordinate, carrying the net name (doc pattern: create([x,y,x2,y], 'NET')).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/16_wire_stubs.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = { wires: [] };

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const schTab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (schTab) {
  await eda.dmt_EditorControl.activateDocument(schTab.tabId);
  await sleep(700);
}

// [pinX, pinY, stubEndX, stubEndY, net]
const stubs = [
  [380, 510, 320, 510, '5V'], // J1.1
  [380, 500, 320, 500, 'SIG'], // J1.2
  [380, 490, 320, 490, 'GND'], // J1.3
  [680, 400, 620, 400, '5V'], // R1.1
  [720, 400, 780, 400, 'LED1_A'], // R1.2
  [970, 400, 1030, 400, 'LED1_A'], // LED1.A
  [930, 400, 870, 400, 'GND'], // LED1.K
  [680, 700, 620, 700, '5V'], // R2.1
  [720, 700, 780, 700, 'LED2_A'], // R2.2
  [970, 700, 1030, 700, 'LED2_A'], // LED2.A
  [930, 700, 870, 700, 'GND'], // LED2.K
];

for (const [x1, y1, x2, y2, net] of stubs) {
  try {
    await eda.sch_PrimitiveWire.create([x1, y1, x2, y2], net);
    log.wires.push({ net, ok: true });
  } catch (e) {
    log.wires.push({ net, ok: false, error: String(e) });
  }
}

await sleep(500);
try {
  const nets = await eda.sch_Net.getCurrentProjectAllNets();
  log.netsAfter = nets;
} catch (e) {
  log.netsErr = String(e);
}

await eda.sch_Document.save();
return JSON.stringify(log, null, 1);
