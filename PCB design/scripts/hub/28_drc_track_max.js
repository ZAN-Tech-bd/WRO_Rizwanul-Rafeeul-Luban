// Hub v0.4 PCB (copy of robot 28, D-033) — DRC rule fix (WRITES the project's current DRC rule configuration only; no copper).
// EasyEDA's default rule set "copperThickness1oz" caps track width at 2.54 mm and flagged the user-drawn 3 mm power tracks
// (BAT / DC / V12) as "Physical Error". JLC has no maximum track width, so the cap is raised to 5.08 mm.
// Idempotent (skips when already set), reads the value back.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\28_drc_track_max.js"
const PROJECT = 'WRO_HUB';
const MAX_MM = 5.08;
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /pcb/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no PCB tab' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
{ const _t = Date.now(); while (Date.now() - _t < 500) await eda.dmt_Project.getCurrentProjectInfo(); }
const read = async () => {
  const c = await eda.pcb_Drc.getCurrentRuleConfiguration();
  return { c, row: c && c.config && c.config.Physics && c.config.Physics.Track && c.config.Physics.Track.copperThickness1oz.form.data['1'] };
};
const before = await read();
if (!before.row) return JSON.stringify({ abort: 'Track rule copperThickness1oz not found' });
const was = before.row.maxValue;
if (Math.abs(was - MAX_MM) < 0.001) return JSON.stringify({ skipped: true, maxValue: was, name: before.c.name });
before.row.maxValue = MAX_MM;
const ok = await eda.pcb_Drc.overwriteCurrentRuleConfiguration(before.c.config);
const after = await read();
return JSON.stringify({ name: before.c.name, was, ok, now: after.row.maxValue, min: after.row.minValue });
