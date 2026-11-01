// WRO_HUB P1 SB subtraction evidence (READ-ONLY). Does not activate a different document.
const PROJECT = 'WRO_HUB_REPAIR_ISOLATED_20261027', PAGE = 'a013c67bfde245d3';
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== PROJECT || doc?.uuid !== PAGE || doc?.documentType !== 1)
  throw new Error('Wrong project/schematic context; open WRO_HUB P1');
const components = [];
for (const c of await eda.sch_PrimitiveComponent.getAll()) {
  const pins = (await c.getAllPins()).map(p => ({id: p.getState_PrimitiveId(),
    number: p.getState_PinNumber(), x: p.getState_X(), y: p.getState_Y(),
  })).sort((a, b) => String(a.number).localeCompare(String(b.number)));
  components.push({id: c.getState_PrimitiveId(), designator: c.getState_Designator(),
    x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(), pins});
}
components.sort((a, b) => a.id.localeCompare(b.id));
const wires = (await eda.sch_PrimitiveWire.getAll()).map(w => ({
  id: w.getState_PrimitiveId(), line: w.getState_Line(), net: w.getState_Net(),
})).sort((a, b) => a.id.localeCompare(b.id));
const apiNetNames = (await eda.sch_Net.getAllNetsName()).sort();
const pinNets = [];
for (const c of components) for (const pin of c.pins) {
  const touching = wires.filter(w => {
    const f = w.line.flat(3);
    return (f[0] === pin.x && f[1] === pin.y) ||
      (f[f.length - 2] === pin.x && f[f.length - 1] === pin.y);
  });
  if (touching.length > 1) throw new Error('Multiple wire endpoints at ' + c.designator + '.' + pin.number);
  if (touching.length) pinNets.push({pin: c.designator + '.' + pin.number, net: touching[0].net});
}
pinNets.sort((a, b) => a.pin.localeCompare(b.pin));
// Local half-offline projects can return [] from BOTH schematic net-name APIs.
// Derive the actual named connectivity from pin-attached wires, as hub/10_check.js does.
const nets = [...new Set(pinNets.map(p => p.net))].sort();
const erc = await eda.sch_Drc.check();
return JSON.stringify({project: project.friendlyName, page: PAGE, components, wires, nets,
  apiNetNames, pinNets, erc, visibility: document.visibilityState});
