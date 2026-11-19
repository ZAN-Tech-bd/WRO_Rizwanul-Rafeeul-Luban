// Robot carrier v0.5 — step P4g: rename the net of existing pin stubs (WRITES the schematic).
// v0.5 (2026-10-25): J9.1 MEGA_5V -> +5V (D3 removed, D-022); D2.1 AMP_R -> VAMP_BAT (R1 removed, D-023).
// FIX = {designator: {pinNumber: newNet}}. Finds the wire that starts at each pin and renames it with
// sch_PrimitiveWire.modify(id, {net}). Idempotent: stubs that already carry the new name are skipped. A modify can commit
// late: if the read-back still shows the old name, wait ~8 s and re-run (never retry in the same run).
// Then: 15_check.js (twice) + sch_audit.py, and the user imports the change into the PCB.
// From attendance PCB/scripts/16_relabel.js (pcb-pipeline). Gates: WRO_Robot, schematic tab, window visible.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\16_relabel_v05.js"
const FIX = { J9: { 1: '+5V' }, D2: { 1: 'VAMP_BAT' } };
const PROJECT = 'WRO_Robot';
// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
if (document.visibilityState !== 'visible') return JSON.stringify({ abort: 'EasyEDA window is hidden: bring it to the front' });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(500);
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = (await eda.sch_PrimitiveComponent.get(ids)).filter((c) => c && c.getState_Designator && FIX[c.getState_Designator()]);
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const ends = (w) => { const f = (w.getState_Line() || []).flat(3); return [[f[0], f[1]], [f[f.length - 2], f[f.length - 1]]]; };
const res = { renamed: [], already: [], noStub: [], missingParts: Object.keys(FIX).filter((d) => !comps.some((c) => c.getState_Designator() === d)) };
for (const c of comps) {
  const des = c.getState_Designator();
  for (const p of (await c.getAllPins()) || []) {
    const pin = String(p.getState_PinNumber()), want = FIX[des][pin];
    if (!want) continue;
    const px = p.getState_X(), py = p.getState_Y();
    const w = wires.find((x) => ends(x).some(([ex, ey]) => Math.abs(ex - px) < 0.5 && Math.abs(ey - py) < 0.5));
    if (!w) { res.noStub.push(des + '.' + pin); continue; }
    if (w.getState_Net() === want) { res.already.push(des + '.' + pin + '=' + want); continue; }
    await eda.sch_PrimitiveWire.modify(w.getState_PrimitiveId(), { net: want });
    res.renamed.push(des + '.' + pin + ': ' + w.getState_Net() + ' -> ' + want);
  }
}
await sleep(3000);
// read back
const now = (await eda.sch_PrimitiveWire.getAll()) || [];
res.readBack = [];
for (const c of comps) for (const p of (await c.getAllPins()) || []) {
  if (!FIX[c.getState_Designator()][String(p.getState_PinNumber())]) continue;
  const w = now.find((x) => ends(x).some(([ex, ey]) => Math.abs(ex - p.getState_X()) < 0.5 && Math.abs(ey - p.getState_Y()) < 0.5));
  res.readBack.push(c.getState_Designator() + '.' + p.getState_PinNumber() + '=' + (w ? w.getState_Net() : 'none'));
}
await eda.sch_Document.save();
return JSON.stringify(res);
