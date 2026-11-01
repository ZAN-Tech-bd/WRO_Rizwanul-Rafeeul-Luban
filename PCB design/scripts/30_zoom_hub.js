// Zoom the (now-active) WRO_HUB window to all parts, sample designators, save.
// Run AFTER selecting the window: curl -X POST .../eda-windows/select
const out = {};
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids || []);
out.count = comps.length;
out.designators = [];
let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
for (const c of comps) {
  out.designators.push((typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?');
  const x = typeof c.getState_X === 'function' ? c.getState_X() : NaN;
  const y = typeof c.getState_Y === 'function' ? c.getState_Y() : NaN;
  if (!Number.isNaN(x)) {
    if (x < minX) minX = x;
    if (x > maxX) maxX = x;
  }
  if (!Number.isNaN(y)) {
    if (y < minY) minY = y;
    if (y > maxY) maxY = y;
  }
}
out.bounds = {
  minX: Number.isFinite(minX) ? minX : null,
  minY: Number.isFinite(minY) ? minY : null,
  maxX: Number.isFinite(maxX) ? maxX : null,
  maxY: Number.isFinite(maxY) ? maxY : null,
};
try {
  await eda.dmt_EditorControl.zoomToAllPrimitives();
  out.zoomed = true;
} catch (e) {
  out.zoomErr = String(e).slice(0, 80);
}
try {
  await eda.sch_Document.save();
  out.saved = true;
} catch (e) {
  out.saveErr = String(e).slice(0, 80);
}
return JSON.stringify(out, null, 1);
