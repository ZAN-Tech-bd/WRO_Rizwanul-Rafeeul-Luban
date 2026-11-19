// Debug — does the schematic have a real netlist? (schematic tab gets activated)
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/13_debug_netlist.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const trim = (v, d) => {
  if (d <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 15).map((x) => trim(x, d - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = trim(v[k], d - 1);
    return o;
  }
  if (typeof v === 'string' && v.length > 100) return v.slice(0, 100) + '…';
  return v;
};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const schTab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (schTab) {
  await eda.dmt_EditorControl.activateDocument(schTab.tabId);
  await sleep(700);
}

const out = { netlistMethods: Object.getOwnPropertyNames(Object.getPrototypeOf(eda.sch_Netlist)) };

try {
  out.allNets = await eda.sch_Net.getCurrentProjectAllNets();
} catch (e) {
  out.allNetsErr = String(e);
}
try {
  out.allNetsName = await eda.sch_Net.getAllNetsName();
} catch (e) {
  out.allNetsNameErr = String(e);
}

// try common netlist getters on sch_Netlist
for (const m of ['getNetlist', 'getCurrentProjectNetlist', 'getNetList']) {
  if (typeof eda.sch_Netlist[m] === 'function') {
    try {
      out['nl_' + m] = await eda.sch_Netlist[m]();
    } catch (e) {
      out['nl_' + m + '_err'] = String(e);
    }
  }
}

return JSON.stringify(trim(out, 4), null, 1);
