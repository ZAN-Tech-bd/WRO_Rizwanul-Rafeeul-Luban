// WRITE isolated repair PCB ONLY: move R5 out of the microSD module strip (user 2026-10-27).
// Exactly: R5 (1890,932.5,rot0) -> (1890,1335,rot0); delete old SERVO_IN stub
// 39b1d74c0d7108f7; move R5 designator to (1890,1335). The new SERVO_IN pad lands on the
// existing net track 918299703502a0e0 (same-net overlap = connection). No other edits.
const T = {"project": "ea32443fed8f1fd5989b3f8520da07a81d65c35e38ca7f144b04f7dc31730e1f", "name": "WRO_HUB_REPAIR_ISOLATED_20261027", "pcb": "8efe5b895e7646ea"};
const FROM = [1890, 932.5], TO = [1890, 1335];
const STUB = '39b1d74c0d7108f7';
const STUB_ENDS = [[1687.24, 932.5], [1645, 974.74]];
const near = (a, b) => Math.abs(a - b) < 0.01;
const p = await eda.dmt_Project.getCurrentProjectInfo(), d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (p?.uuid !== T.project || p?.friendlyName !== T.name || d?.uuid !== T.pcb || d?.documentType !== 3) throw new Error('Wrong repair PCB');
if (document.visibilityState !== 'visible') throw new Error('Keep repair PCB visible');
const comps = await eda.pcb_PrimitiveComponent.getAll();
const r5 = comps.find(c => c.getState_Designator() === 'R5');
if (!r5) throw new Error('R5 missing');
const at = (x, y) => near(r5.getState_X(), x) && near(r5.getState_Y(), y);
if (!at(...TO)) {
  if (!at(...FROM) || Math.round(((r5.getState_Rotation() % 360) + 360) % 360) !== 0)
    throw new Error('R5 moved since plan; re-plan');
  const pins = await r5.getAllPins();
  const nets = Object.fromEntries(pins.map(q => [q.getState_PadNumber(), q.getState_Net()]));
  if (nets['1'] !== 'SERVO_IN' || nets['2'] !== 'GND') throw new Error('R5 nets changed');
  if (!(await eda.pcb_PrimitiveComponent.modify(r5.getState_PrimitiveId(), { x: TO[0], y: TO[1] })))
    throw new Error('R5 move failed');
}
const lines = await eda.pcb_PrimitiveLine.getAll();
const stub = lines.find(l => l.getState_PrimitiveId() === STUB);
if (stub) {
  const s = [stub.getState_StartX(), stub.getState_StartY()], e = [stub.getState_EndX(), stub.getState_EndY()];
  const endsOk = (near(s[0], STUB_ENDS[0][0]) && near(s[1], STUB_ENDS[0][1]) && near(e[0], STUB_ENDS[1][0]) && near(e[1], STUB_ENDS[1][1])) ||
                 (near(s[0], STUB_ENDS[1][0]) && near(s[1], STUB_ENDS[1][1]) && near(e[0], STUB_ENDS[0][0]) && near(e[1], STUB_ENDS[0][1]));
  if (!endsOk || stub.getState_Net() !== 'SERVO_IN' || Number(stub.getState_Layer()) !== 2) throw new Error('Stub changed; re-plan');
  if (!(await eda.pcb_PrimitiveLine.delete(STUB))) throw new Error('Stub delete failed');
}
const attrs = await eda.pcb_PrimitiveAttribute.getAll(r5.getState_PrimitiveId());
const des = attrs.find(a => a.getState_Key() === 'Designator');
if (!des) throw new Error('R5 designator missing');
if (!near(des.getState_X(), TO[0]) || !near(des.getState_Y(), TO[1])) {
  if (!await eda.pcb_PrimitiveAttribute.modify(des.getState_PrimitiveId(), { x: TO[0], y: TO[1] }))
    throw new Error('Designator move failed');
}
if (!(await eda.pcb_Document.save())) throw new Error('Save failed');
// readback
const r5b = (await eda.pcb_PrimitiveComponent.getAll()).find(c => c.getState_Designator() === 'R5');
const pinsB = await r5b.getAllPins();
const pin1 = pinsB.find(q => q.getState_PadNumber() === '1');
const pin2 = pinsB.find(q => q.getState_PadNumber() === '2');
const stubGone = !(await eda.pcb_PrimitiveLine.getAll()).some(l => l.getState_PrimitiveId() === STUB);
return JSON.stringify({
  r5: [r5b.getState_X(), r5b.getState_Y(), r5b.getState_Rotation()],
  pin1: [pin1.getState_X(), pin1.getState_Y(), pin1.getState_Net()],
  pin2: [pin2.getState_X(), pin2.getState_Y(), pin2.getState_Net()],
  stubDeleted: stubGone,
  designator: [des.getState_X ? (await eda.pcb_PrimitiveAttribute.getAll(r5b.getState_PrimitiveId())).find(a => a.getState_Key() === 'Designator').getState_X() : 0],
  saved: true,
});
