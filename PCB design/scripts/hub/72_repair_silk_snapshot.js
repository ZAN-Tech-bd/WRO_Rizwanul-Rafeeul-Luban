// H-004 READ-ONLY: exact live silk bboxes, visible designators, and pad geometry for scoped cleanup.
const p=await eda.dmt_Project.getCurrentProjectInfo(), d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.friendlyName!=='WRO_HUB_REPAIR_ISOLATED_20261027'||d?.uuid!=='8efe5b895e7646ea'||d?.documentType!==3)throw new Error('Wrong PCB');
const texts=[];
for(const s of await eda.pcb_PrimitiveString.getAll()){
  texts.push({id:s.getState_PrimitiveId(),text:s.getState_Text(),
    box:await eda.pcb_Primitive.getPrimitivesBBox([s.getState_PrimitiveId()])});
}
const designators=[];
for(const a of await eda.pcb_PrimitiveAttribute.getAll()){
  if(a.getState_Key()!=='Designator')continue;
  try{designators.push({id:a.getState_PrimitiveId(),text:a.getState_Value(),
    box:await eda.pcb_Primitive.getPrimitivesBBox([a.getState_PrimitiveId()])});}catch(e){}
}
const pads=(await eda.pcb_PrimitivePad.getAll()).map(q=>({id:q.getState_PrimitiveId(),
  x:q.getState_X(),y:q.getState_Y(),pad:q.getState_Pad(),net:q.getState_Net()}));
return JSON.stringify({texts,designators,pads});
