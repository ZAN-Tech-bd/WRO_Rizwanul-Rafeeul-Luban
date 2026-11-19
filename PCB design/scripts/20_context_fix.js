// Debug 5 — force document-context rebind: toggle tabs, verify which doc the API sees.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/20_context_fix.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
const startTab = tabs.find((t) => /start/i.test(t.title));
const schTab = tabs.find((t) => /schematic/i.test(t.title));
const pcbTab = tabs.find((t) => /^pcb/i.test(t.title));
const out = {};

// toggle away and back
if (startTab) {
  await eda.dmt_EditorControl.activateDocument(startTab.tabId);
  await sleep(800);
}
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await sleep(1200);
}

// which doc does the API think is current?
try {
  const pcb = await eda.dmt_Pcb.getCurrentPcbInfo();
  out.currentPcb = pcb && { uuid: pcb.uuid, name: pcb.name };
} catch (e) {
  out.pcbInfoErr = String(e);
}

// inventory again
try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  out.compCount = (ids || []).length;
  const comps = await eda.pcb_PrimitiveComponent.get(ids || []);
  out.comps = comps.map((c) => ({
    d: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
    x: typeof c.getState_X === 'function' ? c.getState_X() : '?',
    y: typeof c.getState_Y === 'function' ? c.getState_Y() : '?',
  }));
} catch (e) {
  out.compErr = String(e);
}

try {
  out.nets = await eda.pcb_Net.getAllNetsName();
} catch (e) {
  out.netErr = String(e);
}

return JSON.stringify(out, null, 1);
