// Isolated working copy ONLY: normalize existing visible silk to manufacturing text/stroke minimums.
// This pass changes style only. A measured-bbox placement pass follows; no components/copper modified.
const T=__TARGET__,PLAN=__SILK_STYLE_PLAN__;
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==T.project||d?.uuid!==T.pcb||d?.documentType!==3)throw new Error('Wrong repair project/PCB');
if(document.visibilityState!=='visible')throw new Error('Keep repair PCB visible');
const strings=await eda.pcb_PrimitiveString.getAll(),attrs=await eda.pcb_PrimitiveAttribute.getAll();
for(const a of PLAN){
  const q=(a.kind==='STRING'?strings:attrs).find(q=>q.getState_PrimitiveId()===a.id);
  const text=a.kind==='STRING'?q?.getState_Text():q?.getState_Value();
  if(!q||text!==a.text)throw new Error('Silk identity changed; nothing written');
}
for(const a of PLAN){
  const api=a.kind==='STRING'?eda.pcb_PrimitiveString:eda.pcb_PrimitiveAttribute;
  if(!await api.modify(a.id,{fontSize:a.fontSize,lineWidth:a.lineWidth}))throw new Error('Silk style failed');
}
if(!await eda.pcb_Document.save())throw new Error('Save failed');
return JSON.stringify({visibleSilkStyled:PLAN.length,minimumHeightMil:40,minimumStrokeMil:6,
  componentMoves:0,copperEdits:0,next:'Measure actual text bboxes and relocate only conflicting silk'});
