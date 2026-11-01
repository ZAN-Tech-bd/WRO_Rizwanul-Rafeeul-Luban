// WRITE Robot PCB1 ONLY: delete the single orphaned GND stitching via 44628080eda6d090.
// It sits inside the pour-excluded antenna strip and nothing connects to it (the user's
// rework removed its stub, D-032 note); no fill can ever reach it. Guarded against the
// saved snapshot; fresh readback + full-export diff required after.
const VIA = '44628080eda6d090';
const PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864';
const PCB = '643ce99542452c8f';
const p = await eda.dmt_Project.getCurrentProjectInfo();
if (p?.uuid !== PROJECT || p?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.documentType !== 3 || d?.uuid !== PCB) throw new Error('Open Robot PCB1');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const vias = await eda.pcb_PrimitiveVia.getAll();
const target = vias.find(v => v.getState_PrimitiveId() === VIA);
if (!target) throw new Error('Target via absent (already removed?)');
const near = (a, b) => Math.abs(a - b) < 0.0001;
const nearSize = (a, b) => Math.abs(a - b) < 0.11;  // live-getter unit rounding (~0.05 mil), hub nearSize
if (String(target.getState_Net()) !== 'GND' ||
    !near(target.getState_X(), 1145) || !near(target.getState_Y(), 3670) ||
    !nearSize(target.getState_HoleDiameter(), 15.75) || !nearSize(target.getState_Diameter(), 31.5))
  throw new Error('Via identity mismatch: ' + JSON.stringify(
    { net: target.getState_Net(), x: target.getState_X(), y: target.getState_Y(),
      hole: target.getState_HoleDiameter(), dia: target.getState_Diameter() }));
const before = vias.length;
if (!await eda.pcb_PrimitiveVia.delete([VIA])) throw new Error('Via delete returned falsy');
const after = (await eda.pcb_PrimitiveVia.getAll());
if (after.length !== before - 1 || after.some(v => v.getState_PrimitiveId() === VIA))
  throw new Error('Fresh readback failed: via still present or count wrong');
if (!await eda.pcb_Document.save()) throw new Error('Save failed; do not retry');
return JSON.stringify({ deleted: VIA, viasBefore: before, viasAfter: after.length, saved: true });
