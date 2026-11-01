// Hub v0.3 PCB — step 4a: inventory every footprint (read-only): position, rotation, layer, bbox, pads + nets.
// Line per part: DES|footprint|x,y|rot|layer|bbox w x h (centre offset dx,dy)|pad:net@dx,dy;...  (pad offsets
// relative to the component origin, mil). Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/20_pcb_inventory.js"
const t = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((t && t.tabs) || []).find((x) => /pcb/i.test(x.title));
if (tab) { await eda.dmt_EditorControl.activateDocument(tab.tabId); await new Promise((r) => setTimeout(r, 600)); }
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.pcb_PrimitiveComponent.get(ids);
const r1 = (v) => Math.round(v * 10) / 10;
const lines = [];
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator();
  const x = c.getState_X(), y = c.getState_Y();
  let bb = null;
  try { bb = await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]); } catch (e) { bb = null; }
  const fp = (c.getState_Footprint() || {}).name || '?';
  const pads = (await c.getAllPins()) || [];
  const ps = pads.map((p) => `${p.getState_PadNumber()}:${p.getState_Net() || '-'}@${r1(p.getState_X() - x)},${r1(p.getState_Y() - y)}`);
  const box = bb ? `${r1(bb.maxX - bb.minX)}x${r1(bb.maxY - bb.minY)}(${r1((bb.maxX + bb.minX) / 2 - x)},${r1((bb.maxY + bb.minY) / 2 - y)})` : '?';
  lines.push(`${des}|${fp}|${r1(x)},${r1(y)}|${c.getState_Rotation()}|${c.getState_Layer()}|${box}|${ps.join(';')}`);
}
return lines.sort().join('\n');
