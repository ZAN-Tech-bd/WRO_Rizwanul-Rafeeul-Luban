// Debug 7 — probe EVERY connected EasyEDA window: which one holds the WRO_HUB schematic?
// Bypasses the helper: targets each window directly via POST /execute {code, windowId}.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/28_windows_probe.js"
const BRIDGE = 'http://127.0.0.1:49620';

const windows = await (await fetch(`${BRIDGE}/eda-windows`)).json();
const out = { windows: windows.windows, probes: [] };

const PROBE = `
const o = {};
try { const si = await eda.dmt_Schematic.getCurrentSchematicInfo(); o.schName = si && si.name; o.pageCount = si && si.page ? si.page.length : '?'; } catch (e) { o.schErr = String(e).slice(0, 60); }
try { const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId(); o.compCount = (ids || []).length; } catch (e) { o.compErr = String(e).slice(0, 60); }
try { o.netCount = ((await eda.sch_Net.getAllNetsName()) || []).length; } catch (e) { o.netErr = String(e).slice(0, 60); }
try { const t = await eda.dmt_EditorControl.getSplitScreenTree(); o.tabs = (t.tabs || []).map(x => x.title); } catch (e) { o.tabErr = String(e).slice(0, 60); }
return JSON.stringify(o);
`;

for (const w of windows.windows || []) {
  try {
    const res = await fetch(`${BRIDGE}/execute`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ code: PROBE, windowId: w.windowId }),
      signal: AbortSignal.timeout(20000),
    });
    const body = await res.json();
    let parsed = body.result;
    try {
      parsed = JSON.parse(body.result);
    } catch {}
    out.probes.push({ windowId: w.windowId, active: w.active, ...parsed });
  } catch (e) {
    out.probes.push({ windowId: w.windowId, active: w.active, error: String(e) });
  }
}

return JSON.stringify(out, null, 1);
