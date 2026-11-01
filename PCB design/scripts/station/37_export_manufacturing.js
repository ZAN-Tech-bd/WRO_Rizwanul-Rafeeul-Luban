// READ-ONLY manufacturing export for the station board: strict DRC + Gerber + BOM. No order.
const PROJECT = '2d2da5292f828ea674cc52b5833bd8a226b03f5595240d96a18801ebe8c8acea';
const PCB = 'fdcb20516f125209';
const p = await eda.dmt_Project.getCurrentProjectInfo();
const d0 = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (p?.uuid !== PROJECT) throw new Error('Wrong project');
if (d0?.uuid !== PCB) await eda.dmt_EditorControl.openDocument(PCB);
const d = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (d?.uuid !== PCB || d?.documentType !== 3) throw new Error('Wrong PCB');
if (document.visibilityState !== 'visible') throw new Error('Keep PCB visible');
const toB64 = async f => { const b = new Uint8Array(await f.arrayBuffer()); let s = '';
  for (let i = 0; i < b.length; i += 0x8000) s += String.fromCharCode.apply(null, b.subarray(i, i + 0x8000));
  return btoa(s); };
const drc = await eda.pcb_Drc.check(true, false, true);
const gerber = await eda.pcb_ManufactureData.getGerberFile('station_pcb_final', false, undefined,
  { integerNumber: 4, decimalNumber: 6 },
  { metallicDrillingInformation: true, nonMetallicDrillingInformation: true, drillTable: false, flyingProbeTestingFile: false });
if (!gerber) throw new Error('Gerber export failed');
const bom = await eda.pcb_ManufactureData.getBomFile('station_bom', 'csv');
return JSON.stringify({ drc, gerber: { name: gerber.name, size: gerber.size, b64: await toB64(gerber) },
  bom: bom ? { name: bom.name, size: bom.size, b64: await toB64(bom) } : null });
