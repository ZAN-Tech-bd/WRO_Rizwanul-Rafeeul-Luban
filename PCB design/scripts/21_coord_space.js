// Debug 6 — understand PCB coordinate space + huge-region sweep.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/21_coord_space.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const pcbTab = ((tree && tree.tabs) || []).find((t) => /^pcb/i.test(t.title));
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await sleep(1000);
}
const out = {};

try {
  out.canvasOrigin = await eda.pcb_Document.getCanvasOrigin();
} catch (e) {
  out.originErr = String(e);
}

try {
  out.conv00 = await eda.pcb_Document.convertCanvasOriginToDataOrigin(0, 0);
} catch (e) {
  out.convErr = String(e);
}

// huge region sweep
try {
  const prims = await eda.pcb_Document.getPrimitivesInRegion(-1e9, -1e9, 1e9, 1e9);
  const list = Array.isArray(prims) ? prims : [];
  out.hugeRegionCount = list.length;
  const byType = {};
  for (const p of list) {
    const t =
      typeof p.getState_PrimitiveType === 'function'
        ? p.getState_PrimitiveType()
        : p.primitiveType || typeof p;
    byType[t] = (byType[t] || 0) + 1;
  }
  out.byType = byType;
} catch (e) {
  out.hugeErr = String(e);
}

try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  out.compIds = (ids || []).length;
} catch (e) {
  out.compErr = String(e);
}

return JSON.stringify(out, null, 1);
