// WRITE Robot PCB1 ONLY: restore GND stitching via 48f8c94d642fddbd to its planned
// position (1970,3680). It shifted (5,-20) mil during the fill batch run (observed in the
// mid-pour export; both positions verified >=15 mil from all foreign copper, so the board
// was never at risk). Restoring keeps the board exactly on the source-locked pour plan.
const VIA = '48f8c94d642fddbd';
const PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864';
const PCB = '643ce99542452c8f';
const p = await eda.dmt_Project.getCurrentProjectInfo();
if (p?.uuid !== PROJECT || p?.friendlyName !== 'WRO_Robot') throw new Error('Wrong Robot project');
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.documentType !== 3 || d?.uuid !== PCB) throw new Error('Open Robot PCB1');
if (document.visibilityState !== 'visible') throw new Error('Keep EasyEDA visible');
const vias = await eda.pcb_PrimitiveVia.getAll();
const v = vias.find(q => q.getState_PrimitiveId() === VIA);
if (!v) throw new Error('Via absent');
const near = (a, b) => Math.abs(a - b) < 0.0001;
const nearSize = (a, b) => Math.abs(a - b) < 0.11;
if (String(v.getState_Net()) !== 'GND' || !near(v.getState_X(), 1975) || !near(v.getState_Y(), 3660) ||
    !nearSize(v.getState_HoleDiameter(), 15.75) || !nearSize(v.getState_Diameter(), 31.5))
  throw new Error('Unexpected via state: ' + JSON.stringify({ net: v.getState_Net(), x: v.getState_X(), y: v.getState_Y() }));
if (!await eda.pcb_PrimitiveVia.modify(VIA, { x: 1970, y: 3680 })) throw new Error('Via restore failed');
const again = (await eda.pcb_PrimitiveVia.getAll()).find(q => q.getState_PrimitiveId() === VIA);
if (!near(again.getState_X(), 1970) || !near(again.getState_Y(), 3680)) throw new Error('Restore readback failed');
if (!await eda.pcb_Document.save()) throw new Error('Save failed');
return JSON.stringify({ restored: VIA, to: [1970, 3680], saved: true });
