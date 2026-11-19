// READ-ONLY manufacturing inspection for the isolated repair board. No design mutation, no order.
const T={"project": "ea32443fed8f1fd5989b3f8520da07a81d65c35e38ca7f144b04f7dc31730e1f", "pcb": "8efe5b895e7646ea"};
const p=await eda.dmt_Project.getCurrentProjectInfo(),d0=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==T.project)throw new Error('Wrong project');
if(d0?.uuid!==T.pcb)await eda.dmt_EditorControl.openDocument(T.pcb);
const d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(d?.uuid!==T.pcb||d?.documentType!==3)throw new Error('Wrong PCB');
if(document.visibilityState!=='visible')throw new Error('Keep PCB visible');
const b64=f=>{const b=new Uint8Array(f?0:0);};
const toB64=async f=>{const bytes=new Uint8Array(await f.arrayBuffer());let s='';for(let i=0;i<bytes.length;i+=0x8000)s+=String.fromCharCode.apply(null,bytes.subarray(i,i+0x8000));return btoa(s);};
const drc=await eda.pcb_Drc.check(true,false,true);
const gerber=await eda.pcb_ManufactureData.getGerberFile('WRO_HUB_FIXED_review',false,undefined,
  {integerNumber:4,decimalNumber:6},
  {metallicDrillingInformation:true,nonMetallicDrillingInformation:true,drillTable:false,flyingProbeTestingFile:false});
if(!gerber)throw new Error('Gerber export failed');
const bom=await eda.pcb_ManufactureData.getBomFile('WRO_HUB_FIXED_bom','csv');
return JSON.stringify({drc,gerber:{name:gerber.name,size:gerber.size,b64:await toB64(gerber)},
  bom:bom?{name:bom.name,size:bom.size,b64:await toB64(bom)}:null});
