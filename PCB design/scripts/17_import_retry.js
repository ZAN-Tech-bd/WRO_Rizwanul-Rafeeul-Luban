// Step 7d — retry PCB import now that named wires exist.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/17_import_retry.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
const pcbTab = tabs.find((t) => /^pcb/i.test(t.title));
if (!pcbTab) return JSON.stringify({ fail: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
await sleep(800);

try {
  log.imported = await eda.pcb_Document.importChanges();
} catch (e) {
  log.importErr = String(e);
}
await sleep(1500);

try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  const comps = await eda.pcb_PrimitiveComponent.get(ids || []);
  log.pcbComponents = [];
  for (const c of comps) {
    log.pcbComponents.push({
      designator: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
      x: typeof c.getState_X === 'function' ? c.getState_X() : '?',
      y: typeof c.getState_Y === 'function' ? c.getState_Y() : '?',
    });
  }
} catch (e) {
  log.invErr = String(e);
}

// PCB nets too
try {
  log.pcbNets = await eda.pcb_Net.getAllNetsName();
} catch (e) {
  log.pcbNetsErr = String(e);
}

await eda.pcb_Document.save();
return JSON.stringify(log, null, 1);
