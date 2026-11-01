// Step 8 — verify what's on the PCB after the ECO dialog was applied.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/18_pcb_inventory.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const pcbTab = ((tree && tree.tabs) || []).find((t) => /^pcb/i.test(t.title));
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await sleep(700);
}

const out = {};
try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  const comps = await eda.pcb_PrimitiveComponent.get(ids || []);
  out.components = [];
  for (const c of comps) {
    out.components.push({
      designator: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
      x: typeof c.getState_X === 'function' ? c.getState_X() : '?',
      y: typeof c.getState_Y === 'function' ? c.getState_Y() : '?',
      rotation: typeof c.getState_Rotation === 'function' ? c.getState_Rotation() : '?',
      layer: typeof c.getState_Layer === 'function' ? c.getState_Layer() : '?',
    });
  }
} catch (e) {
  out.compErr = String(e);
}

try {
  out.nets = await eda.pcb_Net.getAllNetsName();
} catch (e) {
  out.netErr = String(e);
}

await eda.pcb_Document.save();
return JSON.stringify(out, null, 1);
