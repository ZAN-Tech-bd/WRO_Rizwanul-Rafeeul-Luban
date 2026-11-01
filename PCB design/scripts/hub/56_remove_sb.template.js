// WRO_HUB PCB1 SB1–SB7 subtraction ONLY (WRITES). Generated from the verified live baseline.
// Preconditions: fresh saved .epro2 backup; RED export tests; visible editor; exact project/PCB;
// all 84 component geometries/pad nets and free strings/nets unchanged; no routed copper.
// No component moves/modifies, imports, net deletes, copper deletes, pours or outline calls.
// Do not replay on a changed board. If interrupted, inspect/export; never retry blindly.
const EXPECTED = __EXPECTED_SB_BASELINE__;
const PROJECT = 'WRO_HUB', PCB = 'ad35aed4d7610fd2';
const fail = message => { throw new Error(message); };
if (document.visibilityState !== 'visible') fail('EasyEDA window is hidden');
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== PROJECT || project?.uuid !== EXPECTED.projectUuid ||
    doc?.uuid !== PCB || doc?.documentType !== 3 || doc?.parentProjectUuid !== project.uuid)
  fail('Wrong project/PCB context');
const getComponents = async () => {
  const result = [];
  for (const c of await eda.pcb_PrimitiveComponent.getAll()) {
    const pads = (await c.getAllPins()).map(p => ({
      id: p.getState_PrimitiveId(), number: p.getState_PadNumber(), net: p.getState_Net(),
      x: p.getState_X(), y: p.getState_Y(),
    })).sort((a, b) => String(a.number).localeCompare(String(b.number)));
    result.push({id: c.getState_PrimitiveId(), designator: c.getState_Designator(),
      x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(),
      footprint: c.getState_Footprint(), pads});
  }
  return result.sort((a, b) => a.designator.localeCompare(b.designator));
};
const current = await getComponents();
const strings = (await eda.pcb_PrimitiveString.getAll()).map(s => ({
  id: s.getState_PrimitiveId(), text: s.getState_Text(), x: s.getState_X(), y: s.getState_Y(),
}));
const nets = (await eda.pcb_Net.getAllNetsName()).sort();
const eq = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const targets = current.filter(c => /^SB[1-7]$/.test(c.designator));
const survivors = EXPECTED.components.filter(c => !/^SB[1-7]$/.test(c.designator));
if (current.length === 77 && targets.length === 0 && eq(current, survivors))
  return JSON.stringify({alreadyRemoved: true, components: 77, writes: 0});
if (current.length !== 84 || !eq(current, EXPECTED.components)) fail('Component/pad geometry differs from backup');
if (targets.length !== 7 || new Set(targets.map(c => c.designator)).size !== 7) fail('Not exactly SB1–SB7');
if (!eq(strings, EXPECTED.strings) || strings.some(s => /bypass|\bSB\d+\b/i.test(s.text)))
  fail('Silkscreen changed or bypass/free SB note needs separate review');
if (!eq(nets, EXPECTED.nets)) fail('Net list differs from backup');
if ((await eda.pcb_PrimitiveLine.getAll()).length !== 0) fail('Copper found: requires new stub review');
for (let k = 1; k <= 7; k++) {
  const c = targets.find(c => c.designator === 'SB' + k);
  if (c.footprint?.uuid !== '2ca2a36c91af7b3f' || c.pads.length !== 2 ||
      c.pads[0].net !== 'S' + k + '_DIN' || c.pads[1].net !== 'S' + k + '_DOUT')
    fail('Unexpected bypass footprint or net mapping: SB' + k);
}
const started = Date.now();
if (!await eda.pcb_PrimitiveComponent.delete(targets.map(c => c.id))) fail('Component delete failed: inspect, do not retry');
const after = await getComponents();
if (!eq(after, survivors)) fail('Post-delete survivor mismatch: inspect backup before proceeding');
if (!eq((await eda.pcb_Net.getAllNetsName()).sort(), EXPECTED.nets)) fail('Post-delete net definitions changed');
const attributes = await eda.pcb_PrimitiveAttribute.getAll();
if (attributes.some(a => /^SB\d+$/.test(String(a.getState_Value())))) fail('SB designator attribute remains');
if (!await eda.pcb_Document.save()) fail('Save failed: deletion is not yet persistence-verified');
return JSON.stringify({deleted: targets.map(c => ({designator: c.designator, id: c.id, pads: c.pads})),
  before: current.length, after: after.length, copperDeleted: 0, freeSilkDeleted: 0,
  netsPreserved: nets.length, saved: true, elapsedMs: Date.now() - started,
  persistenceVerification: 'REQUIRED: full project export and test_sb_removal.py'});
