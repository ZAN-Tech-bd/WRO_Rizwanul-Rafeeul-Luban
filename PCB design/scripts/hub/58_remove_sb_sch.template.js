// WRO_HUB P1 SB1–SB7 subtraction ONLY (WRITES schematic, never PCB).
// User explicitly authorized this follow-up to eliminate the PCB/schematic Netlist Error.
// The RED acceptance test proves all 14 allowlisted 30-unit stubs touch exactly one SB pin,
// no other symbol pin and no other wire. Baseline equality guards against subsequent changes.
// No imports, net deletes, edits to survivors, or same-run retries.
const EXPECTED = __EXPECTED_SB_SCHEMATIC__;
const PROJECT = 'WRO_HUB', PAGE = '7e64f4e0d47bf001';
const fail = message => { throw new Error(message); };
if (document.visibilityState !== 'visible') fail('EasyEDA window is hidden');
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== PROJECT || project?.uuid !== EXPECTED.projectUuid ||
    doc?.uuid !== PAGE || doc?.documentType !== 1 || doc?.parentProjectUuid !== project.uuid)
  fail('Wrong project/schematic context');
const componentsNow = async () => {
  const result = [];
  for (const c of await eda.sch_PrimitiveComponent.getAll()) {
    const pins = (await c.getAllPins()).map(p => ({id: p.getState_PrimitiveId(),
      number: p.getState_PinNumber(), x: p.getState_X(), y: p.getState_Y(),
    })).sort((a, b) => String(a.number).localeCompare(String(b.number)));
    result.push({id: c.getState_PrimitiveId(), designator: c.getState_Designator(),
      x: c.getState_X(), y: c.getState_Y(), rotation: c.getState_Rotation(), pins});
  }
  return result.sort((a, b) => a.id.localeCompare(b.id));
};
const wiresNow = async () => (await eda.sch_PrimitiveWire.getAll()).map(w => ({
  id: w.getState_PrimitiveId(), line: w.getState_Line(), net: w.getState_Net(),
})).sort((a, b) => a.id.localeCompare(b.id));
const eq = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const components = await componentsNow(), wires = await wiresNow();
if (!eq(components, EXPECTED.components) || !eq(wires, EXPECTED.wires))
  fail('Schematic geometry or wire mapping differs from verified baseline');
if (!eq((await eda.sch_Net.getAllNetsName()).sort(), EXPECTED.nets)) fail('Schematic nets changed');
const targets = components.filter(c => /^SB[1-7]$/.test(c.designator));
const stubIds = new Set(EXPECTED.stubIds);
if (targets.length !== 7 || new Set(targets.map(c => c.designator)).size !== 7 || stubIds.size !== 14)
  fail('Target/stub count mismatch');
const stubs = wires.filter(w => stubIds.has(w.id));
if (stubs.length !== 14) fail('Allowlisted wire missing');
for (const c of targets) {
  if (c.pins.length !== 2) fail('SB symbol is not two-pin');
  for (const pin of c.pins) {
    const matches = stubs.filter(w => {
      const f = w.line.flat(3);
      return f.length === 4 && ((f[0] === pin.x && f[1] === pin.y) || (f[2] === pin.x && f[3] === pin.y));
    });
    const expectedNet = 'S' + c.designator.slice(2) + (pin.number === '1' ? '_DIN' : '_DOUT');
    if (matches.length !== 1 || matches[0].net !== expectedNet) fail('SB stub mapping changed');
  }
}
if (!await eda.sch_PrimitiveWire.delete(stubs.map(w => w.id))) fail('Wire delete failed: inspect, do not retry');
if (!await eda.sch_PrimitiveComponent.delete(targets.map(c => c.id))) fail('Symbol delete failed: inspect, do not retry');
const remaining = await componentsNow(), remainingWires = await wiresNow();
if (!eq(remaining, EXPECTED.components.filter(c => !/^SB[1-7]$/.test(c.designator))))
  fail('Surviving schematic symbol/pin geometry changed');
if (!eq(remainingWires, EXPECTED.wires.filter(w => !stubIds.has(w.id)))) fail('Unapproved wire change');
if (!eq((await eda.sch_Net.getAllNetsName()).sort(), EXPECTED.nets)) fail('Net definition lost');
if (!await eda.sch_Document.save()) fail('Schematic save failed');
return JSON.stringify({deletedSymbols: targets.map(c => ({designator: c.designator, id: c.id})),
  deletedStubs: stubs, remainingSymbolsWithFrame: remaining.length, remainingWires: remainingWires.length,
  netsPreserved: EXPECTED.nets.length, saved: true, pcbWrites: 0,
  persistenceVerification: 'REQUIRED: final full-project export and 13 acceptance checks'});
