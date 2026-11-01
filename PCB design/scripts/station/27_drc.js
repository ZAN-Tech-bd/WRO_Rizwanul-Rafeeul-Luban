// Station board v0.3 PCB — strict DRC with the verbose error list (READ-ONLY), summarised per category.
// pcb_Drc.check(strict, ui, true) returns [{name: category, list: [{name: net | rule, list: [error, ...]}]}];
// error.pos is in 10-mil units (x10 = mil). "Connection Error" = unrouted; everything else must be zero to order.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\27_drc.js"
const PROJECT = 'station_pcb';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);   // pcb_* calls fail while a schematic tab is active
{ const _t = Date.now(); while (Date.now() - _t < 600) await eda.dmt_Project.getCurrentProjectInfo(); }
const r = await eda.pcb_Drc.check(true, false, true);
const cats = Array.isArray(r) ? r : [];
const out = { passed: cats.length === 0, categories: {} };
const flat = (node) => (node && Array.isArray(node.list) ? node.list.flatMap((n) => (n && n.errorType ? [n] : flat(n))) : []);
for (const c of cats) {
  const errs = flat(c);
  const entry = { count: errs.length };
  if (c.name === 'Connection Error') {
    entry.nets = {};
    for (const e of errs) entry.nets[e.net] = (entry.nets[e.net] || 0) + 1;
  } else {
    entry.samples = errs.slice(0, 40).map((e) => [e.errorType, e.ruleName, e.obj1 && e.obj1.suffix, e.obj2 && e.obj2.suffix,
      e.pos ? Math.round(e.pos.x * 10) + ',' + Math.round(e.pos.y * 10) : ''].filter(Boolean).join(' | '));
  }
  out.categories[c.name] = entry;
}
return JSON.stringify(out, null, 1);
