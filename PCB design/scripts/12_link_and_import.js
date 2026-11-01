// Step 7c — diagnose Board linkage and import with explicit schematic UUID.
// "If the import fails or the free PCB has no schematic UUID passed in, false is returned"
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/12_link_and_import.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

// current PCB info (PCB tab should be active)
try {
  log.pcbInfo = await eda.dmt_Pcb.getCurrentPcbInfo();
} catch (e) {
  log.pcbInfoErr = String(e);
}
try {
  log.boards = await eda.dmt_Board.getAllBoardsInfo();
} catch (e) {
  log.boardsErr = String(e);
}

// get schematic uuid (activate schematic tab briefly)
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
const schTab = tabs.find((t) => /schematic/i.test(t.title));
const pcbTab = tabs.find((t) => /^pcb/i.test(t.title));
let schUuid;
if (schTab) {
  await eda.dmt_EditorControl.activateDocument(schTab.tabId);
  await sleep(700);
  try {
    const si = await eda.dmt_Schematic.getCurrentSchematicInfo();
    schUuid = si && (si.uuid || si.schematicUuid);
    log.schInfo = si;
  } catch (e) {
    log.schInfoErr = String(e);
  }
}

// back to PCB, import with explicit uuid
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await sleep(700);
  try {
    log.imported = await eda.pcb_Document.importChanges(schUuid);
  } catch (e) {
    log.importErr = String(e);
  }
  await sleep(1200);
  try {
    const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
    const comps = await eda.pcb_PrimitiveComponent.get(ids);
    log.pcbComponents = comps.map((c) => ({
      designator: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
    }));
  } catch (e) {
    log.invErr = String(e);
  }
  await eda.pcb_Document.save();
}

function trim(v, d) {
  if (d <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 10).map((x) => trim(x, d - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = trim(v[k], d - 1);
    return o;
  }
  if (typeof v === 'string' && v.length > 120) return v.slice(0, 120) + '…';
  return v;
}
return JSON.stringify(trim(log, 4), null, 1);
