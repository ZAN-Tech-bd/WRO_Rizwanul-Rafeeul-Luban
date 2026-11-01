// READ-ONLY: exact live geometry for reviewing the user's manual station layout.
// Do not apply the historical placement/route scripts to this revision.
// Run through the bridge and redirect the result into data/<snapshot>.json.
const project = await eda.dmt_Project.getCurrentProjectInfo();
if (!project || project.friendlyName !== 'station_pcb') throw new Error('Wrong project: station_pcb required');
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tabs = tree.tabs.filter((t) => t.data && t.data.doctype === 3 && t.data.creator === 'fdcb20516f125209');
if (tabs.length !== 1) throw new Error('Expected exactly the station PCB1 tab');
await eda.dmt_EditorControl.activateDocument(tabs[0].tabId);
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const padState = (p) => ({
  id: p.getState_PrimitiveId(), num: String(p.getState_PadNumber()), net: p.getState_Net() || '',
  x: p.getState_X(), y: p.getState_Y(), rotation: p.getState_Rotation(), layer: Number(p.getState_Layer()),
  shape: p.getState_Pad(), hole: p.getState_Hole(), plated: p.getState_Metallization(),
});
const components = [];
for (const c of await eda.pcb_PrimitiveComponent.getAll()) {
  components.push({id: c.getState_PrimitiveId(), des: c.getState_Designator(),
    x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(), layer: Number(c.getState_Layer()),
    footprint: c.getState_Footprint(), bbox: await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]),
    pads: (await c.getAllPins()).map(padState)});
}
const polylines = (await eda.pcb_PrimitivePolyline.getAll()).map((p) => ({
  id:p.getState_PrimitiveId(), layer:Number(p.getState_Layer()), net:p.getState_Net() || '',
  source:p.getState_Polygon().getSource(), width:p.getState_LineWidth(),
}));
const pads = (await eda.pcb_PrimitivePad.getAll()).map(padState);
const fills = (await eda.pcb_PrimitiveFill.getAll()).map((p) => ({id:p.getState_PrimitiveId(),
  layer:Number(p.getState_Layer()), net:p.getState_Net() || '', source:p.getState_ComplexPolygon().getSource()}));
const pours = (await eda.pcb_PrimitivePour.getAll()).map((p) => ({id:p.getState_PrimitiveId(),
  layer:Number(p.getState_Layer()), net:p.getState_Net() || '', source:p.getState_ComplexPolygon().getSource()}));
const regions = (await eda.pcb_PrimitiveRegion.getAll()).map((p) => ({id:p.getState_PrimitiveId(),
  layer:Number(p.getState_Layer()), source:p.getState_ComplexPolygon().getSource()}));
const copper = {};
const group = (net) => copper[net] || (copper[net] = {segs:[], vias:[], arcs:[]});
for (const p of await eda.pcb_PrimitiveLine.getAll()) {
  if (![1, 2].includes(Number(p.getState_Layer()))) continue;
  group(p.getState_Net() || '').segs.push([Number(p.getState_Layer()),p.getState_StartX(),p.getState_StartY(),
    p.getState_EndX(),p.getState_EndY(),p.getState_LineWidth(),p.getState_PrimitiveId()]);
}
for (const p of await eda.pcb_PrimitiveArc.getAll()) {
  if (![1, 2].includes(Number(p.getState_Layer()))) continue;
  group(p.getState_Net() || '').arcs.push([Number(p.getState_Layer()),p.getState_StartX(),p.getState_StartY(),
    p.getState_EndX(),p.getState_EndY(),p.getState_ArcAngle(),p.getState_LineWidth(),p.getState_PrimitiveId()]);
}
for (const p of await eda.pcb_PrimitiveVia.getAll()) group(p.getState_Net() || '').vias.push([
  p.getState_X(),p.getState_Y(),p.getState_HoleDiameter(),p.getState_Diameter(),p.getState_PrimitiveId()]);
return {project:project.friendlyName,projectUuid:project.uuid,tabId:tabs[0].tabId,
  components, pads, polylines, fills, pours, regions, copper};
