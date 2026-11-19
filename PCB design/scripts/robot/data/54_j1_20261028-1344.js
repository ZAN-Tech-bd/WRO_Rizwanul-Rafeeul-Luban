// WRITE ONE already-open, verified WRO_Robot project-footprint tab only.
// No navigation, device association, component change, pad creation/deletion or retry.
// Canvas verification is NOT persistence evidence: a fresh full export is mandatory.
const C = {"project":"7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864","pcb":"643ce99542452c8f","footprints":[{"uuid":"eac10ca20e6778ba","pads":{"e10":{"num":"1","holeBefore":181.102,"padBefore":216.5,"xBefore":-141.7,"yBefore":0,"holeAfter":157.48,"padAfter":null},"e11":{"num":"2","holeBefore":181.102,"padBefore":216.5,"xBefore":141.7,"yBefore":0,"holeAfter":157.48,"padAfter":null}}}]};
const p0 = await eda.dmt_Project.getCurrentProjectInfo();
if (p0?.uuid !== C.project || p0?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
if (!Array.isArray(C.footprints) || C.footprints.length !== 1) throw new Error('Exactly one footprint per run');
const fp = C.footprints[0];
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (doc?.documentType !== 4 || doc.uuid !== fp.uuid || doc.tabId !== fp.uuid + '@' + C.project)
  throw new Error('Not the project footprint editor for ' + fp.uuid);
const near = (a, b) => Number.isFinite(a) && Number.isFinite(b) && Math.abs(a - b) < 0.00001;
const read = p => ({ id: p.getState_PrimitiveId(), num: String(p.getState_PadNumber()),
  x: p.getState_X(), y: p.getState_Y(), hole: p.getState_Hole(), pad: p.getState_Pad() });
const before = (await eda.pcb_PrimitivePad.getAll()).map(read);
if (before.length !== 2 || before.length !== Object.keys(fp.pads).length) throw new Error('Pad count mismatch');
// Validate EVERY configuration entry and existing pad before issuing the first write.
for (const b of before) {
  const w = fp.pads[b.id];
  if (!w || !Number.isFinite(w.holeAfter) || w.holeAfter <= 0 ||
      !(w.padAfter === null || (Number.isFinite(w.padAfter) && w.padAfter > w.holeAfter)))
    throw new Error('Invalid target dimensions for ' + b.id);
  if (b.num !== String(w.num) || !near(b.x, w.xBefore) || !near(b.y, w.yBefore) ||
      b.hole?.[0] !== 'ROUND' || !near(b.hole[1], w.holeBefore) ||
      b.pad?.[0] !== 'ELLIPSE' || !near(b.pad[1], w.padBefore) || !near(b.pad[2], w.padBefore))
    throw new Error('Pad identity or before geometry changed for ' + b.id);
}
for (const b of before) {
  const w = fp.pads[b.id];
  const props = { hole: ['ROUND', w.holeAfter] };
  if (w.padAfter !== null) props.pad = ['ELLIPSE', w.padAfter, w.padAfter];
  if (!await eda.pcb_PrimitivePad.modify(b.id, props)) throw new Error('Pad modification failed: ' + b.id);
}
const after = (await eda.pcb_PrimitivePad.getAll()).map(read);
if (after.length !== before.length) throw new Error('Pad count changed after edit');
for (const b of after) {
  const old = before.find(q => q.id === b.id), w = fp.pads[b.id];
  const targetPad = w?.padAfter === null ? w.padBefore : w?.padAfter;
  if (!old || b.num !== old.num || !near(b.x, old.x) || !near(b.y, old.y) ||
      b.hole?.[0] !== 'ROUND' || !near(b.hole[1], w.holeAfter) ||
      b.pad?.[0] !== 'ELLIPSE' || !near(b.pad[1], targetPad) || !near(b.pad[2], targetPad))
    throw new Error('Fresh readback failed for ' + b.id + '; do not retry');
}
if (!await eda.pcb_Document.save()) throw new Error('Footprint save failed; do not retry');
return JSON.stringify({ footprint: fp.uuid, before, after, saveReturned: true,
  verification: 'REQUIRED: fresh full export and exact change allowlist before proceeding' });
