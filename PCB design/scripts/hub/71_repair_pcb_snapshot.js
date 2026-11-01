// WRO_HUB_REPAIR_ISOLATED_20261027 PCB1 SB subtraction evidence (READ-ONLY; strict DRC, no geometry edits or repour).
// Run via the bridge; capture the returned JSON before and after 56_remove_sb.js.
const PROJECT = 'WRO_HUB_REPAIR_ISOLATED_20261027', PCB = '8efe5b895e7646ea';
const project = await eda.dmt_Project.getCurrentProjectInfo();
const documentInfo = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== PROJECT || documentInfo?.uuid !== PCB || documentInfo?.documentType !== 3)
  throw new Error('Wrong project/PCB context; open WRO_HUB_REPAIR_ISOLATED_20261027 PCB1');
const components = [];
for (const c of await eda.pcb_PrimitiveComponent.getAll()) {
  const pads = (await c.getAllPins()).map(p => ({
    id: p.getState_PrimitiveId(), number: p.getState_PadNumber(), net: p.getState_Net(),
    x: p.getState_X(), y: p.getState_Y(),
  })).sort((a, b) => String(a.number).localeCompare(String(b.number)));
  components.push({id: c.getState_PrimitiveId(), designator: c.getState_Designator(),
    x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(),
    footprint: c.getState_Footprint(), pads});
}
components.sort((a, b) => a.designator.localeCompare(b.designator));
const strings = (await eda.pcb_PrimitiveString.getAll()).map(s => ({
  id: s.getState_PrimitiveId(), text: s.getState_Text(), x: s.getState_X(), y: s.getState_Y(),
}));
const attributes = (await eda.pcb_PrimitiveAttribute.getAll()).map(a => ({
  id: a.getState_PrimitiveId(), value: a.getState_Value(),
}));
const nets = (await eda.pcb_Net.getAllNetsName()).sort();
const lineCount = (await eda.pcb_PrimitiveLine.getAll()).length;
const drc = await eda.pcb_Drc.check(true, false, true);
return JSON.stringify({project: project.friendlyName, pcb: PCB, components, strings, attributes,
  nets, lineCount, drc, visibility: document.visibilityState});
