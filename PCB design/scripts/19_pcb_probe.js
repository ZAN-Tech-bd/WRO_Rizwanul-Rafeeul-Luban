// Debug 4 — read the PCB via multiple paths after ECO apply (canvas shows footprints).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/19_pcb_probe.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const trim = (v, d) => {
  if (d <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 12).map((x) => trim(x, d - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = trim(v[k], d - 1);
    return o;
  }
  if (typeof v === 'string' && v.length > 60) return v.slice(0, 60) + '…';
  return v;
};

const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = (tree && tree.tabs) || [];
const pcbTab = tabs.find((t) => /^pcb/i.test(t.title));
const out = { tabs: tabs.map((t) => t.title) };
if (pcbTab) {
  await eda.dmt_EditorControl.activateDocument(pcbTab.tabId);
  await sleep(1500);
}

// path 1: component manager
try {
  const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
  out.compIds = (ids || []).length;
  if (ids && ids.length) {
    const comps = await eda.pcb_PrimitiveComponent.get(ids);
    out.comps = comps.map((c) => ({
      d: (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?',
      x: typeof c.getState_X === 'function' ? c.getState_X() : '?',
      y: typeof c.getState_Y === 'function' ? c.getState_Y() : '?',
    }));
  }
} catch (e) {
  out.compErr = String(e);
}

// path 2: region query (huge region)
try {
  const prims = await eda.pcb_Document.getPrimitivesInRegion(-100000, -100000, 100000, 100000);
  const list = Array.isArray(prims) ? prims : prims && prims.primitives ? prims.primitives : [];
  out.regionCount = list.length;
  const byType = {};
  for (const p of list) {
    const t =
      typeof p.getState_PrimitiveType === 'function'
        ? p.getState_PrimitiveType()
        : p.primitiveType || '?';
    byType[t] = (byType[t] || 0) + 1;
  }
  out.regionByType = byType;
  // designators of components found via region
  out.regionDesignators = list
    .filter((p) => typeof p.getState_Designator === 'function')
    .map((p) => p.getState_Designator());
} catch (e) {
  out.regionErr = String(e);
}

return JSON.stringify(trim(out, 3), null, 1);
