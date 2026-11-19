// Station board v0.3 (copied from attendance 15b) — detailed ERC (read-only): runs the schematic DRC with the panel shown and returns its
// [Fatal]/[Error]/[Warn]/[Info] lines. sch_Drc.check() alone only returns true/false (or {type, count} items), so
// the details come from EasyEDA's DRC panel text (scripts run inside the page).
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\station\15b_erc_panel.js"
const PROJECT = 'station_pcb';
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(800);
let r;
try { r = await eda.sch_Drc.check(false, true, true); } catch (e) { r = 'ERR ' + e; }
await sleep(1500);
const lines = ((document.body && document.body.innerText) || '').split('\n').map((s) => s.trim())
  .filter((s) => /^\[(Fatal|Error|Warn|Info)\]/.test(s));
const count = (k) => lines.filter((s) => s.startsWith('[' + k + ']')).length;
return JSON.stringify({ result: r, fatal: count('Fatal'), error: count('Error'), warn: count('Warn'), info: count('Info'),
  lines: lines.slice(0, 20).map((s) => s.slice(0, 160)) });
