// H-004 WRITES only R4/R6/R13 + 73 existing station/title strings + their three designators.
// No additions/deletions, no routes/nets/footprints/outline/pours/schematic operations.
const EXPECTED=__CLEANUP_BASELINE__;
const POSES=__CLEANUP_POSES__;
const TEXTS=__CLEANUP_TEXTS__;
const DES=__CLEANUP_DESIGNATORS__;
const fail=m=>{throw new Error(m);};
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.friendlyName!=='WRO_HUB'||d?.uuid!=='ad35aed4d7610fd2'||d?.documentType!==3)fail('Wrong PCB context');
if(document.visibilityState!=='visible')fail('EasyEDA window hidden');
const inventory=async()=>{
  const all=[];
  for(const c of await eda.pcb_PrimitiveComponent.getAll()){
    const pads=(await c.getAllPins()).map(q=>({id:q.getState_PrimitiveId(),number:q.getState_PadNumber(),net:q.getState_Net(),x:q.getState_X(),y:q.getState_Y()}))
      .sort((a,b)=>String(a.number).localeCompare(String(b.number)));
    all.push({id:c.getState_PrimitiveId(),designator:c.getState_Designator(),x:c.getState_X(),y:c.getState_Y(),rotation:c.getState_Rotation(),footprint:c.getState_Footprint(),pads});
  }
  return all.sort((a,b)=>a.designator.localeCompare(b.designator));
};
const eq=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const before=await inventory();
if(!eq(before,EXPECTED.components))fail('Component/pad state changed since backup; nothing written');
const strings=(await eda.pcb_PrimitiveString.getAll()).map(s=>({id:s.getState_PrimitiveId(),text:s.getState_Text(),x:s.getState_X(),y:s.getState_Y()}));
if(!eq(strings,EXPECTED.strings))fail('Free silk changed since backup; nothing written');
if(Object.keys(POSES).sort().join(',')!=='R13,R4,R6'||Object.keys(TEXTS).length!==73)fail('Scope mismatch');
const attrs=await eda.pcb_PrimitiveAttribute.getAll();
for(const [id,a] of Object.entries(DES)){
  const found=attrs.find(q=>q.getState_PrimitiveId()===id);
  if(!found||found.getState_Key()!=='Designator'||found.getState_Value()!==a.text)fail('Designator identity mismatch');
}
for(const [id,t] of Object.entries(TEXTS))if(strings.find(s=>s.id===id)?.text!==t.text)fail('Text identity mismatch');
for(const c of before.filter(c=>POSES[c.designator])){
  const [x,y,rotation]=POSES[c.designator];
  if(!await eda.pcb_PrimitiveComponent.modify(c.id,{x,y,rotation}))fail('Move failed: '+c.designator);
}
for(const [id,t] of Object.entries(TEXTS)){
  if(!await eda.pcb_PrimitiveString.modify(id,{x:t.x,y:t.y,rotation:t.rotation,fontSize:t.fontSize,lineWidth:t.lineWidth}))fail('Text move failed: '+id);
}
for(const [id,a] of Object.entries(DES)){
  if(!await eda.pcb_PrimitiveAttribute.modify(id,{x:a.x,y:a.y,rotation:0,fontSize:32,lineWidth:6}))fail('Designator placement failed: '+a.text);
}
const after=await inventory();
if(!eq(after.filter(c=>!POSES[c.designator]),before.filter(c=>!POSES[c.designator])))fail('Unrelated component changed');
for(const c of after.filter(c=>POSES[c.designator])){
  const [x,y,r]=POSES[c.designator],old=before.find(q=>q.id===c.id);
  if(c.x!==x||c.y!==y||((c.rotation-r)%360+360)%360!==0)fail('Pose readback mismatch');
  if(!eq(c.footprint,old.footprint)||!eq(c.pads.map(q=>[q.id,q.number,q.net]),old.pads.map(q=>[q.id,q.number,q.net])))fail('Footprint/net mapping changed');
}
if(!await eda.pcb_Document.save())fail('Save failed');
return JSON.stringify({movedComponents:Object.keys(POSES),movedFreeTexts:Object.keys(TEXTS).length,
  placedDesignators:Object.values(DES).map(a=>a.text),frozenComponents:74,saved:true,
  next:'Full project export, actual silk readback and DRC comparison REQUIRED'});
