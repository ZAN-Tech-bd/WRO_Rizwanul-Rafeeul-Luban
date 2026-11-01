// H-005 (READ-ONLY): dump every pcb_PrimitiveFill on WRO_HUB PCB1 with its exact create parameters,
// so GND fills can be deleted and later restored byte-exactly. Run AFTER pcb_Document.save() + .epro2 backup.
// Output: JSON {project, pcb, count, byNetLayer, fills:[{id, net, layer, source, fillMode, lineWidth, lock}]}.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/62_gnd_fill_backup.js"
const PROJECT = 'WRO_HUB', PCB = 'ad35aed4d7610fd2';
const p = await eda.dmt_Project.getCurrentProjectInfo();
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (p?.friendlyName !== PROJECT || d?.uuid !== PCB || d?.documentType !== 3) throw new Error('Wrong PCB context');
const fills = (await eda.pcb_PrimitiveFill.getAll()) || [];
const out = [];
for (const f of fills) {
  const cp = f.getState_ComplexPolygon();
  const source = (cp && typeof cp.getSource === 'function' && cp.getSource()) || (cp && cp.polygon);
  if (!Array.isArray(source) || source.length < 4) throw new Error('Fill ' + f.getState_PrimitiveId() + ' has no polygon source');
  out.push({
    id: f.getState_PrimitiveId(),
    net: f.getState_Net(),
    layer: f.getState_Layer(),
    source,
    fillMode: f.getState_FillMode(),
    lineWidth: f.getState_LineWidth(),
    lock: !!f.getState_PrimitiveLock(),
  });
}
const byNetLayer = {};
for (const f of out) { const k = f.net + '|L' + f.layer; byNetLayer[k] = (byNetLayer[k] || 0) + 1; }
return JSON.stringify({ project: p.friendlyName, pcb: PCB, count: out.length, byNetLayer, fills: out });
