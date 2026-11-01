// WRITE isolated repair PCB ONLY: two +3V3 segment endpoints and one new full-width segment.
const T=__TARGET__,P=__PLAN__;
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==T.project||d?.uuid!==T.pcb||d?.documentType!==3)throw new Error('Wrong repair PCB');
if(document.visibilityState!=='visible')throw new Error('Keep repair PCB visible');
const lines=await eda.pcb_PrimitiveLine.getAll();
const near=(a,b)=>Math.abs(a-b)<.01;
const same=(l,q)=>(near(l.getState_StartX(),q.x1)&&near(l.getState_StartY(),q.y1)&&near(l.getState_EndX(),q.x2)&&near(l.getState_EndY(),q.y2))||
  (near(l.getState_StartX(),q.x2)&&near(l.getState_StartY(),q.y2)&&near(l.getState_EndX(),q.x1)&&near(l.getState_EndY(),q.y1));
for(const [id,q] of Object.entries(P.modify)){
 const l=lines.find(l=>l.getState_PrimitiveId()===id);
 if(!l||l.getState_Net()!==q.net||l.getState_Layer()!==q.layer||Math.abs(l.getState_LineWidth()-q.width)>.11||(!same(l,P.before[id])&&!same(l,q)))throw new Error('RF track changed');
}
for(const [id,q] of Object.entries(P.modify)){
 if(!await eda.pcb_PrimitiveLine.modify(id,{startX:q.x1,startY:q.y1,endX:q.x2,endY:q.y2}))throw new Error('RF modify failed');
}
let added=0;
for(const q of P.add){
 if(lines.some(l=>l.getState_Net()===q.net&&l.getState_Layer()===q.layer&&same(l,q)))continue;
 if(!await eda.pcb_PrimitiveLine.create(q.net,q.layer,q.x1,q.y1,q.x2,q.y2,q.width,false))throw new Error('RF create refused; no retry');
 added++;
}
if(!await eda.pcb_Document.save())throw new Error('Save failed');
return JSON.stringify({modifiedTrackIds:Object.keys(P.modify),added,componentMoves:0,viaChanges:0,next:'Regenerate affected GND before release'});
