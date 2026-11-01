// Fix — select the WRO_HUB window (the one with 91 components) as the bridge target
// and zoom its canvas to the parts so they're front-and-center for review.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/29_focus_wro_hub.js"
const BRIDGE = 'http://127.0.0.1:49620';
const HUB_WINDOW = 'ae08333f-d9cc-4a3d-8cf7-640376c44c8b';

// 1) make it the active window for all future bridge commands
const sel = await (
  await fetch(`${BRIDGE}/eda-windows/select`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ windowId: HUB_WINDOW }),
  })
).json();
const out = { selected: sel };

// 2) in that window: bounds + designator sample + zoom to parts
const PROBE = `
const out = {};
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids || []);
let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
out.designators = [];
for (const c of comps) {
  out.designators.push((typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?');
  const x = c.getState_X ? c.getState_X() : NaN;
  const y = c.getState_Y ? c.getState_Y() : NaN;
  if (!Number.isNaN(x)) { if (x < minX) minX = x; if (x > maxX) maxX = x; }
  if (!Number.isNaN(y)) { if (y < minY) minY = y; if (y > maxY) maxY = y; }
}
out.bounds = { minX, minY, maxX, maxY, count: comps.length };
try { await eda.dmt_EditorControl.zoomToAllPrimitives(); out.zoomed = true; } catch (e) { out.zoomErr = String(e).slice(0, 80); }
try { await eda.sch_Document.save(); out.saved = true; } catch (e) { out.saveErr = String(e).slice(0, 80); }
return JSON.stringify(out);
`;

const res = await fetch(`${BRIDGE}/execute`, {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ code: PROBE, windowId: HUB_WINDOW }),
  signal: AbortSignal.timeout(30000),
});
const body = await res.json();
try {
  out.probe = JSON.parse(body.result);
} catch {
  out.probeRaw = body.result;
}

return JSON.stringify(out, null, 1);
