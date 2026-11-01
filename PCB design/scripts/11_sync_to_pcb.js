// Step 7b — ensure components are flagged AddIntoBom/AddIntoPcb (API placement may
// default them OFF), then re-run the PCB import. Switches tabs programmatically.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/11_sync_to_pcb.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

// 1) find tab ids
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
log.tabs = tabs.map((t) => ({ title: t.title, tabId: t.tabId }));
const schTab = tabs.find((t) => /schematic|原理图/i.test(t.title));
const pcbTab = tabs.find((t) => /pcb/i.test(t.title) && !/panel/i.test(t.title));
if (!schTab || !pcbTab) return JSON.stringify({ ...log, fail: 'tabs not found' }, null, 1);

// 2) schematic context: set flags
await eda.dmt_EditorControl.activateDocument(schTab.tabId);
await sleep(800);
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
log.flagged = [];
for (const c of comps) {
  try {
    const before = typeof c.getState_AddIntoPcb === 'function' ? c.getState_AddIntoPcb() : 'n/a';
    const s = c.toSync();
    if (typeof s.setState_AddIntoBom === 'function') s.setState_AddIntoBom(true);
    if (typeof s.setState_AddIntoPcb === 'function') s.setState_AddIntoPcb(true);
    log.flagged.push({ des: c.getState_Designator(), before });
  } catch (e) {
    log.flagged.push({ error: String(e) });
  }
}
await eda.sch_Document.save();

// 3) PCB context: import + inventory
await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
await sleep(800);
try {
  log.imported = await eda.pcb_Document.importChanges();
} catch (e) {
  log.importError = String(e);
}
await sleep(1200);

try {
  const ids2 = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  const comps2 = await eda.pcb_PrimitiveComponent.get(ids2);
  log.pcbComponents = [];
  for (const c of comps2) {
    log.pcbComponents.push({
      designator: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
      x: typeof c.getState_X === 'function' ? c.getState_X() : undefined,
      y: typeof c.getState_Y === 'function' ? c.getState_Y() : undefined,
    });
  }
} catch (e) {
  log.inventoryError = String(e);
}

await eda.pcb_Document.save();
return JSON.stringify(log, null, 1);
