// H-003 WRITES only the 37 verified-unplaced station components on WRO_HUB PCB1.
// Generated from a fresh backup + tested live-footprint model. No import/route/pour/outline/silk calls.
// Preconditions are checked across ALL 77 components and their pads before the first write.
// Idempotent if all target poses already match. No blind retries; export and inspect on any failure.
const EXPECTED = __STATION_BASELINE__;
const TARGETS = __STATION_TARGETS__;
const fail = m => {throw new Error(m);};
const p=await eda.dmt_Project.getCurrentProjectInfo(), doc=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.friendlyName!=='WRO_HUB'||doc?.uuid!=='ad35aed4d7610fd2'||doc?.documentType!==3||doc?.parentProjectUuid!==p.uuid)
  fail('Wrong project/PCB');
if(document.visibilityState!=='visible')fail('EasyEDA hidden');
const inventory=async()=>{
  const all=[];
  for(const c of await eda.pcb_PrimitiveComponent.getAll()){
    const pads=(await c.getAllPins()).map(q=>({id:q.getState_PrimitiveId(),number:q.getState_PadNumber(),net:q.getState_Net(),x:q.getState_X(),y:q.getState_Y()}))
      .sort((a,b)=>String(a.number).localeCompare(String(b.number)));
    all.push({id:c.getState_PrimitiveId(),designator:c.getState_Designator(),x:c.getState_X(),y:c.getState_Y(),
      rotation:c.getState_Rotation(),footprint:c.getState_Footprint(),pads});
  }
  return all.sort((a,b)=>a.designator.localeCompare(b.designator));
};
const before=await inventory(), eq=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const at=(c,t)=>c.x===t[0]&&c.y===t[1]&&((c.rotation-t[2])%360+360)%360===0;
const fixed=before.filter(c=>!TARGETS[c.designator]);
if(!eq(fixed,EXPECTED.filter(c=>!TARGETS[c.designator])))fail('Frozen component or its pads changed');
if(before.length!==77||Object.keys(TARGETS).length!==37)fail('Unexpected component count');
if(before.filter(c=>TARGETS[c.designator]).every(c=>at(c,TARGETS[c.designator])))
  return JSON.stringify({alreadyPlaced:37,writes:0});
if(!eq(before,EXPECTED))fail('Board changed since backup: re-inventory, do not replay');
for(const c of before.filter(c=>TARGETS[c.designator]))if(c.x<=4695)fail(c.designator+' is not unplaced');
const moved=[];
for(const c of before.filter(c=>TARGETS[c.designator])){
  const [x,y,rotation]=TARGETS[c.designator];
  const changed=await eda.pcb_PrimitiveComponent.modify(c.id,{x,y,rotation});
  if(!changed)fail(c.designator+' modify failed: inspect before retry');
  moved.push(c.designator);
}
const after=await inventory();
if(!eq(after.filter(c=>!TARGETS[c.designator]),fixed))fail('Frozen survivor changed');
for(const c of after.filter(c=>TARGETS[c.designator])){
  if(!at(c,TARGETS[c.designator]))fail(c.designator+' pose readback mismatch');
  const b=before.find(q=>q.id===c.id);
  if(!eq(c.footprint,b.footprint)||!eq(c.pads.map(q=>[q.id,q.number,q.net]),b.pads.map(q=>[q.id,q.number,q.net])))
    fail(c.designator+' footprint/pad-net mapping changed');
}
if(!await eda.pcb_Document.save())fail('Save failed');
return JSON.stringify({moved,count:moved.length,frozen:fixed.length,componentCount:after.length,saved:true,
  next:'Full project export + verify_station_placement.py required'});
