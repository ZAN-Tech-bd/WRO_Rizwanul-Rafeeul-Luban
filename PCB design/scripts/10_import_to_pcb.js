// Step 7 — pull schematic changes into the PCB (PCB1 tab must be open & active!)
// then zoom to board outline and save.
// Run (after opening PCB1): node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/10_import_to_pcb.js"
const log = {};

try {
  log.imported = await eda.pcb_Document.importChanges();
} catch (e) {
  log.importError = String(e);
}

await new Promise((r) => setTimeout(r, 1000));

try {
  await eda.pcb_Document.zoomToBoardOutline();
  log.zoomed = true;
} catch (e) {
  log.zoom = String(e);
}

try {
  await eda.pcb_Document.save();
  log.saved = true;
} catch (e) {
  log.save = String(e);
}

// inventory what's now on the board
try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  const comps = await eda.pcb_PrimitiveComponent.get(ids);
  log.components = [];
  for (const c of comps) {
    const d =
      (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?';
    const x = typeof c.getState_X === 'function' ? c.getState_X() : undefined;
    const y = typeof c.getState_Y === 'function' ? c.getState_Y() : undefined;
    log.components.push({ designator: d, x, y });
  }
} catch (e) {
  log.inventoryError = String(e);
}

return JSON.stringify(log, null, 1);
