// READ-ONLY manufacturing export for the final Robot board: strict DRC + Gerber + BOM.
// No design mutation, no order. Hub-81 method, robot identity guards.
const PROJECT = '7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864';
const PCB = '643ce99542452c8f';
const p = await eda.dmt_Project.getCurrentProjectInfo();
const d0 = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (p?.uuid !== PROJECT || p?.friendlyName !== 'WRO_Robot') throw new Error('Wrong project');
if (d0?.uuid !== PCB) await eda.dmt_EditorControl.openDocument(PCB);
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.uuid !== PCB || d?.documentType !== 3) throw new Error('Wrong PCB');
if (document.visibilityState !== 'visible') throw new Error('Keep PCB visible');
const toB64 = async f => { const bytes = new Uint8Array(await f.arrayBuffer()); let s = '';
  for (let i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
  return btoa(s); };
const drc = await eda.pcb_Drc.check(true, false, true);
const gerber = await eda.pcb_ManufactureData.getGerberFile('WRO_Robot_final', false, undefined,
  { integerNumber: 4, decimalNumber: 6 },
  { metallicDrillingInformation: true, nonMetallicDrillingInformation: true, drillTable: false, flyingProbeTestingFile: false });
if (!gerber) throw new Error('Gerber export failed');
const bom = await eda.pcb_ManufactureData.getBomFile('WRO_Robot_bom', 'csv');
return JSON.stringify({ drc,
  gerber: { name: gerber.name, size: gerber.size, b64: await toB64(gerber) },
  bom: bom ? { name: bom.name, size: bom.size, b64: await toB64(bom) } : null });
