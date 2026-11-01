// WRITE isolated recovery PCB: translate only measured conflicting silk. No components or copper.
const T=__TARGET__,PLAN=__PLAN__;
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==T.project||d?.uuid!==T.pcb||d?.documentType!==3)throw new Error('Wrong repair PCB');
if(document.visibilityState!=='visible')throw new Error('Keep repair PCB visible');
const strings=await eda.pcb_PrimitiveString.getAll(),attrs=await eda.pcb_PrimitiveAttribute.getAll();
for(const a of PLAN){
  const q=(a.kind==='STRING'?strings:attrs).find(q=>q.getState_PrimitiveId()===a.id);
  if(!q||(a.kind==='STRING'?q.getState_Text():q.getState_Value())!==a.text)throw new Error('Text identity changed');
  const old=Math.hypot(q.getState_X()-a.before[0],q.getState_Y()-a.before[1])<.01;
  const done=Math.hypot(q.getState_X()-a.x,q.getState_Y()-a.y)<.01;
  if(!old&&!done)throw new Error('Text moved concurrently');
}
for(const a of PLAN){
  const api=a.kind==='STRING'?eda.pcb_PrimitiveString:eda.pcb_PrimitiveAttribute;
  if(!await api.modify(a.id,{x:a.x,y:a.y}))throw new Error('Text translation failed');
}
if(!await eda.pcb_Document.save())throw new Error('Save failed');
return JSON.stringify({silkTranslated:PLAN.length,componentMoves:0,copperEdits:0});
