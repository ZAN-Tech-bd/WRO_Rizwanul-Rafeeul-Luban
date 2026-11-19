// DISABLED: lib_Device.modify is unsafe for the local project library in this editor build.
// It failed and altered unrelated DEVICE metadata. Recover from the verified checkpoint before continuing.
throw new Error('Disabled unsafe project-device API path; use component-level binding overrides only');
// Historical attempted J3 metadata/binding correction, retained for diagnosis. No code below may execute.
const PROJECT='58cf0863ccb9b405a3019aadf223b9df2e72878780f06e4967d8bd8f9eeb441d';
const DEVICE='0b029b3bccb9137c',FP='ad35aed4d7610fd2_0cbe9290d583d889';
const p=await eda.dmt_Project.getCurrentProjectInfo();
if(p?.friendlyName!=='WRO_HUB'||p.uuid!==PROJECT)throw new Error('Wrong project');
if(document.visibilityState!=='visible')throw new Error('Keep EasyEDA visible');
const updated=await eda.lib_Device.modify(DEVICE,PROJECT,'KCD4-203-B',undefined,
  {footprint:{uuid:FP,libraryUuid:PROJECT},model3D:null},
  'KCD4-203-B DPDT power-selector rocker; verify real-part fit and contact action.',
  {name:'KCD4-203-B',manufacturer:null,manufacturerId:'KCD4-203-B',supplier:null,supplierId:null,
   otherProperty:{'LCSC Part Name':'KCD4-203-B DPDT rocker','Supplier Footprint':'Custom plated-slot KCD4-203-B'}});
if(!updated)throw new Error('Project-device update failed; do not import');
await eda.dmt_EditorControl.openDocument('7e64f4e0d47bf001');
const c=(await eda.sch_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
if(!c||c.getState_PrimitiveId()!=='f26ea52ce64d9e40')throw new Error('Schematic J3 identity mismatch');
const attrs=await eda.sch_PrimitiveAttribute.getAll(c.getState_PrimitiveId());
const values={'Footprint':FP,'Name':'KCD4-203-B','Manufacturer':'','Manufacturer Part':'KCD4-203-B',
  'Supplier':'','Supplier Part':'','LCSC Part Name':'KCD4-203-B DPDT rocker','Supplier Footprint':'Custom plated-slot KCD4-203-B'};
for(const a of attrs){
  const key=a.getState_Key();
  if(Object.prototype.hasOwnProperty.call(values,key))
    if(!await eda.sch_PrimitiveAttribute.modify(a.getState_PrimitiveId(),{value:values[key]}))throw new Error('J3 schematic attribute update failed: '+key);
}
if(!await eda.sch_Document.save())throw new Error('Schematic save failed');
const check=(await eda.sch_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
const footprint=check.getState_Footprint();
if(footprint?.uuid!==FP)throw new Error('J3 schematic still resolves the old footprint');
const erc=await eda.sch_Drc.check();
await eda.dmt_EditorControl.openDocument('ad35aed4d7610fd2');
const pcb=(await eda.pcb_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
if(pcb?.getState_Footprint()?.uuid!==FP)throw new Error('PCB switch footprint mismatch');
if(!await eda.pcb_PrimitiveComponent.modify(pcb.getState_PrimitiveId(),
  {name:'KCD4-203-B',manufacturer:'',manufacturerId:'KCD4-203-B',supplier:'',supplierId:'',
   otherProperty:{'LCSC Part Name':'KCD4-203-B DPDT rocker','Supplier Footprint':'Custom plated-slot KCD4-203-B'}}))
  throw new Error('PCB J3 metadata update failed');
if(!await eda.pcb_Document.save())throw new Error('PCB save failed');
return JSON.stringify({projectDeviceUpdated:true,schematicFootprint:footprint,pcbFootprint:pcb.getState_Footprint(),
  manufacturerPart:'KCD4-203-B',oldSupplierPartRemoved:true,erc,importChangesPerformed:false,
  verification:'REQUIRED: export confirms device metadata and schematic override, unchanged pins/nets'});
