// Debug 3 — page-scoped netlist queries. getNetlist expects an iterable (likely page uuids).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/15_debug_netlist2.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const trim = (v, d) => {
  if (d <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 10).map((x) => trim(x, d - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = trim(v[k], d - 1);
    return o;
  }
  if (typeof v === 'string' && v.length > 80) return v.slice(0, 80) + '…';
  return v;
};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const schTab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (schTab) {
  await eda.dmt_EditorControl.activateDocument(schTab.tabId);
  await sleep(700);
}
const out = {};

let pageUuid;
try {
  const p = await eda.dmt_Schematic.getCurrentSchematicPageInfo();
  out.pageInfo = { uuid: p && p.uuid, name: p && p.name };
  pageUuid = p && p.uuid;
} catch (e) {
  out.pageErr = String(e);
}

if (pageUuid) {
  for (const [tag, fn] of [
    ['getNetlist_arr', () => eda.sch_Netlist.getNetlist([pageUuid])],
    ['getNetlist_str', () => eda.sch_Netlist.getNetlist(pageUuid)],
    ['getAllNets_page', () => eda.sch_Net.getAllNets(pageUuid)],
    ['getNet_page', () => eda.sch_Net.getNet(pageUuid)],
  ]) {
    try {
      out[tag] = await fn();
    } catch (e) {
      out[tag + '_err'] = String(e);
    }
  }
}

return JSON.stringify(trim(out, 5), null, 1);
