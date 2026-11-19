// Step 9 — CLEANUP broken netflags (sch) + TIDY LAYOUT + BOARD OUTLINE (PCB).
// Outline: 800x600 mil rect on layer 11. Then dumps pad coords for the router.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/23_layout_outline.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const log = {};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
const schTab = tabs.find((t) => /schematic/i.test(t.title));
const pcbTab = tabs.find((t) => /^pcb/i.test(t.title));

// ── A) schematic: delete the 3 broken netflags ("?" primitives with no pins)
if (schTab) {
  await eda.dmt_EditorControl.activateDocument(schTab.tabId);
  await sleep(700);
  const sids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
  const scomps = await eda.sch_PrimitiveComponent.get(sids || []);
  const junk = [];
  for (const c of scomps) {
    const pins = (await c.getAllPins()) || [];
    const des = (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?';
    if (pins.length === 0 && /\?/.test(String(des))) junk.push(c.getState_PrimitiveId());
  }
  if (junk.length) {
    try {
      await eda.sch_PrimitiveComponent.delete(junk);
      log.deletedNetflags = junk.length;
    } catch (e) {
      log.delErr = String(e);
    }
  }
  await eda.sch_Document.save();
}

// ── B) PCB: tidy layout + outline
if (!pcbTab) return JSON.stringify({ ...log, fail: 'no pcb tab' }, null, 1);
await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
await sleep(800);

const targets = { J1: [100, 300], R1: [300, 180], R2: [300, 420], LED1: [550, 180], LED2: [550, 420] };
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids || []);
log.moved = [];
for (const c of comps) {
  const d = (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?';
  const t = targets[d];
  if (!t) continue;
  try {
    const s = c.toSync();
    if (typeof s.setState_X === 'function') s.setState_X(t[0]);
    if (typeof s.setState_Y === 'function') s.setState_Y(t[1]);
    log.moved.push({ des: d, to: t, ok: true });
  } catch (e) {
    log.moved.push({ des: d, ok: false, error: String(e) });
  }
}

// board outline: 800x600 mil rectangle on layer 11
log.outline = [];
const seg = (x1, y1, x2, y2) => eda.pcb_PrimitiveLine.create('', 11, x1, y1, x2, y2, 10);
try {
  await seg(0, 0, 800, 0);
  await seg(800, 0, 800, 600);
  await seg(800, 600, 0, 600);
  await seg(0, 600, 0, 0);
  log.outline.push({ ok: true });
} catch (e) {
  // retry with string layer
  try {
    await eda.pcb_PrimitiveLine.create('', '11', 0, 0, 800, 0, 10);
    await eda.pcb_PrimitiveLine.create('', '11', 800, 0, 800, 600, 10);
    await eda.pcb_PrimitiveLine.create('', '11', 800, 600, 0, 600, 10);
    await eda.pcb_PrimitiveLine.create('', '11', 0, 600, 0, 0, 10);
    log.outline.push({ ok: true, note: 'string layer' });
  } catch (e2) {
    log.outline.push({ ok: false, error: String(e2) });
  }
}

// dump pad coordinates for routing (num + x + y per component)
log.pads = [];
for (const c of comps) {
  const d = (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?';
  try {
    const pads = await c.getAllPins();
    log.pads.push({
      des: d,
      pads: pads.map((p) => ({
        num: typeof p.getState_PadNumber === 'function' ? p.getState_PadNumber() : typeof p.getState_Number === 'function' ? p.getState_Number() : '?',
        x: typeof p.getState_X === 'function' ? Math.round(p.getState_X() * 100) / 100 : '?',
        y: typeof p.getState_Y === 'function' ? Math.round(p.getState_Y() * 100) / 100 : '?',
      })),
    });
  } catch (e) {
    log.pads.push({ des: d, error: String(e) });
  }
}

await eda.pcb_Document.save();
return JSON.stringify(log, null, 1);
