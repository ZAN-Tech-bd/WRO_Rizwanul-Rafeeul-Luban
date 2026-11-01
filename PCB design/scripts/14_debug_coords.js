// Debug 2 — verify what actually sits at the pin coordinates; check wires; locate labels.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/14_debug_coords.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const trim = (v, d) => {
  if (d <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 8).map((x) => trim(x, d - 1));
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

// 1) wires
try {
  const w = await eda.sch_PrimitiveWire.getAllPrimitiveId();
  out.wireCount = (w || []).length;
} catch (e) {
  out.wireErr = String(e);
}

// 2) what primitive is at J1 pin2 (380,500)?
try {
  const p = await eda.sch_Document.getPrimitiveAtPoint(380, 500);
  out.at_380_500 = trim(p, 3);
} catch (e) {
  out.atErr = String(e);
}

// 3) where did the net labels actually land?
try {
  const ids = await eda.sch_PrimitiveAttribute.getAllPrimitiveId();
  out.labelCount = (ids || []).length;
  const attrs = await eda.sch_PrimitiveAttribute.get(ids || []);
  out.labels = [];
  for (const a of attrs || []) {
    out.labels.push({
      type: typeof a.getState_PrimitiveType === 'function' ? a.getState_PrimitiveType() : '?',
      text: typeof a.getState_Text === 'function' ? a.getState_Text() : typeof a.getState_Net === 'function' ? a.getState_Net() : '?',
      x: typeof a.getState_X === 'function' ? a.getState_X() : '?',
      y: typeof a.getState_Y === 'function' ? a.getState_Y() : '?',
    });
  }
} catch (e) {
  out.labelErr = String(e);
}

return JSON.stringify(trim(out, 4), null, 1);
