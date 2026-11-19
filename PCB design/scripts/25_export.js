// Step 11 — EXPORT the JLCPCB package (BOM + Gerber + Pick&Place).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/25_export.js"
const log = {};

log.pcbMfgMethods = Object.getOwnPropertyNames(Object.getPrototypeOf(eda.pcb_ManufactureData)).filter(
  (m) => m !== 'constructor'
);
log.schMfgMethods = Object.getOwnPropertyNames(Object.getPrototypeOf(eda.sch_ManufactureData)).filter(
  (m) => m !== 'constructor'
);

// find tab activation (PCB context for pcb exports)
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const pcbTab = ((tree && tree.tabs) || []).find((t) => /^pcb/i.test(t.title));
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await new Promise((r) => setTimeout(r, 800));
}

const attempt = async (tag, fn) => {
  try {
    const r = await fn();
    log[tag] = typeof r === 'string' && r.length > 300 ? r.slice(0, 300) + '…' : r;
  } catch (e) {
    log[tag] = 'ERR: ' + String(e);
  }
};

await attempt('bom_csv', () => eda.pcb_ManufactureData.getBomFile('dummy-bom.csv', 'CSV'));
await attempt('gerber', () => eda.pcb_ManufactureData.getGerberFile('dummy-gerber'));
await attempt('position', () => eda.pcb_ManufactureData.getPositionFile('dummy-pos.csv', 'CSV'));

return JSON.stringify(log, null, 1);
