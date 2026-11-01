// Pinned recovery project; ONLY instance attributes. Never call lib_Device.modify or Import Changes.
const T=__RECOVERY_TARGET__;
const p=await eda.dmt_Project.getCurrentProjectInfo();
if(p?.uuid!==T.project||p?.friendlyName!==T.name)throw new Error('Wrong recovery working copy');
if(document.visibilityState!=='visible')throw new Error('Keep the recovery PCB visible');
await eda.dmt_EditorControl.openDocument(T.page);
const j3=(await eda.sch_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
if(j3?.getState_PrimitiveId()!=='f26ea52ce64d9e40')throw new Error('J3 identity mismatch');
const values={'Footprint':T.footprint,'Name':'KCD4-203-B','Manufacturer':'User-supplied',
  'Manufacturer Part':'KCD4-203-B','Supplier':'User-supplied','Supplier Part':'Not applicable',
  'LCSC Part Name':'KCD4-203-B DPDT power-selector rocker','Supplier Footprint':'Custom KCD4-203-B plated-slot footprint'};
const attrs=await eda.sch_PrimitiveAttribute.getAll(j3.getState_PrimitiveId());
for(const [key,value] of Object.entries(values)){
  const a=attrs.find(a=>a.getState_Key()===key);
  if(!a)throw new Error('Expected schematic instance attribute missing: '+key);
  if(!await eda.sch_PrimitiveAttribute.modify(a.getState_PrimitiveId(),{value}))throw new Error('Attribute write failed: '+key);
}
if(!await eda.sch_Document.save())throw new Error('Schematic save failed');
const resolved=(await eda.sch_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3').getState_Footprint();
if(resolved?.uuid!==T.footprint)throw new Error('Schematic instance footprint did not resolve to custom switch');
const erc=await eda.sch_Drc.check();
await eda.dmt_EditorControl.openDocument(T.pcb);
const pcb=(await eda.pcb_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
if(pcb?.getState_Footprint()?.uuid!==T.footprint)throw new Error('PCB switch footprint mismatch');
for(const [key,value] of Object.entries(values)){
  if(key==='Footprint')continue;
  await pcb.setAttribute(key,value,false,false);
}
if(!await eda.pcb_Document.save())throw new Error('PCB save failed');
return JSON.stringify({schematicResolvedFootprint:resolved,pcbFootprint:pcb.getState_Footprint(),erc,
  modifiedOnlyJ3InstanceAttributes:true,libraryDeviceModified:false,importChangesPerformed:false});
