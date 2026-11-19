// READ-ONLY: fabrication-file export for inspection. Does not place an order or modify design geometry.
const project=await eda.dmt_Project.getCurrentProjectInfo();
const doc=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(project?.friendlyName!=='WRO_HUB'||doc?.uuid!=='ad35aed4d7610fd2'||doc?.documentType!==3)throw new Error('Wrong PCB');
const file=await eda.pcb_ManufactureData.getGerberFile('WRO_HUB_review',false,undefined,
  {integerNumber:4,decimalNumber:6},
  {metallicDrillingInformation:true,nonMetallicDrillingInformation:true,drillTable:false,flyingProbeTestingFile:false});
if(!file)throw new Error('Gerber export returned no file');
const bytes=new Uint8Array(await file.arrayBuffer());let text='';
for(let i=0;i<bytes.length;i+=0x8000)text+=String.fromCharCode.apply(null,bytes.subarray(i,i+0x8000));
return JSON.stringify({name:file.name,size:bytes.length,b64:btoa(text)});
