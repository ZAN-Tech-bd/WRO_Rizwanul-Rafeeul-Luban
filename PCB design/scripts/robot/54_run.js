// RETIRED diagnostic evidence: original generated configuration/writer had a field mismatch.
throw new Error('Disabled faulty writer. Build a fresh one-footprint writer using the tested builder.');
// WRITE WRO_Robot project footprints ONLY: XT drills/pads to user-approved D-031 sizes.
// Hub-retry-2 method (proven persistent): openInEditor -> pcb_PrimitivePad.modify -> save.
// J1 XT60: hole 4.0 (pad 5.5 kept). J2/J3 XT30-F + J4 XT30-M: hole 2.5, pad 3.5.
const C = {"project":"7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864","pcb":"643ce99542452c8f","footprints":[{"uuid":"eac10ca20e6778ba","pads":{"e10":{"num":"1","holeBefore":181.102,"padBefore":216.535,"holeAfter":157.48,"padAfter":null},"e11":{"num":"2","holeBefore":181.102,"padBefore":216.535,"holeAfter":157.48,"padAfter":null}}},{"uuid":"9e78c3c3ca74ad42","pads":{"e24":{"num":"1","holeBefore":86.614,"padBefore":125.984,"holeAfter":98.425,"padAfter":137.795},"e25":{"num":"2","holeBefore":86.614,"padBefore":125.984,"holeAfter":98.425,"padAfter":137.795}}},{"uuid":"9197bb2e6c38da76","pads":{"e18":{"num":"1","holeBefore":70.866,"padBefore":98.425,"holeAfter":98.425,"padAfter":137.795},"e19":{"num":"2","holeBefore":70.866,"padBefore":98.425,"holeAfter":98.425,"padAfter":137.795}}}]};
const p0 = await eda.dmt_Project.getCurrentProjectInfo();
if (p0?.uuid !== C.project || p0?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const results = [];
for (const fp of C.footprints) {
  await eda.lib_Footprint.openInEditor(fp.uuid, C.project);
  const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
  if (doc?.documentType !== 4 || doc.uuid !== fp.uuid || doc.tabId !== fp.uuid + '@' + C.project)
    throw new Error('Not the project footprint editor for ' + fp.uuid);
  const before = [];
  for (const pad of await eda.pcb_PrimitivePad.getAll()) {
    const id = pad.getState_PrimitiveId();
    const want = fp.pads[id];
    if (!want) throw new Error('Unexpected pad ' + id + ' in ' + fp.uuid);
    const hole = pad.getState_Hole();
    const dp = pad.getState_DefaultPad ? pad.getState_DefaultPad() : null;
    before.push({ id, num: pad.getState_PadNumber(), hole: hole && hole.width, pad: dp && dp.width });
  }
  if (before.length !== Object.keys(fp.pads).length) throw new Error('Pad count mismatch in ' + fp.uuid);
  for (const b of before) {
    const want = fp.pads[b.id];
    const props = { hole: ['ROUND', want.hole] };
    if (want.pad) props.pad = ['ELLIPSE', want.pad, want.pad];
    if (!await eda.pcb_PrimitivePad.modify(b.id, props)) throw new Error('Pad modify failed ' + fp.uuid + '/' + b.id);
  }
  if (!await eda.pcb_Document.save()) throw new Error('Footprint save failed ' + fp.uuid);
  const after = [];
  for (const pad of await eda.pcb_PrimitivePad.getAll()) {
    const hole = pad.getState_Hole();
    after.push({ id: pad.getState_PrimitiveId(), hole: hole && hole.width, padType: hole && null });
  }
  results.push({ uuid: fp.uuid, before, after });
}
await eda.dmt_EditorControl.openDocument(C.pcb);
if (!await eda.pcb_Document.save()) throw new Error('PCB save failed');
return JSON.stringify({ footprints: results, pcb: C.pcb, reopened: true });
