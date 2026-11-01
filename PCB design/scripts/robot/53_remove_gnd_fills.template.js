// WRITE WRO_Robot PCB1 ONLY: remove exact checkpointed GND Fill primitives from layers1/2.
// No copper, placement, pad, via, net, schematic, image or library edits. Never auto-restore after user edits.
const C=__CONFIG__;
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==C.project||p?.friendlyName!=='WRO_Robot'||d?.uuid!==C.pcb||d?.documentType!==3)throw new Error('Wrong Robot project/PCB');
if(document.visibilityState!=='visible')throw new Error('Keep Robot PCB visible');
const selected=(await eda.pcb_PrimitiveFill.getAll()).filter(f=>f.getState_Net()==='GND'&&[1,2].includes(Number(f.getState_Layer())));
if(!selected.length)return JSON.stringify({deleted:0,alreadyClear:true});
if(selected.length!==C.fills.length)throw new Error('GND fill count changed; rebuild checkpoint');
const expected=new Map(C.fills.map(f=>[f.id,f]));
for(const f of selected){
 const q=expected.get(f.getState_PrimitiveId());
 if(!q||f.getState_Net()!==q.net||Number(f.getState_Layer())!==q.layer||
    JSON.stringify(f.getState_ComplexPolygon().getSource())!==JSON.stringify(q.source)||
    f.getState_FillMode()!==q.fillMode||f.getState_LineWidth()!==q.lineWidth||!!f.getState_PrimitiveLock()!==q.lock)
   throw new Error('GND polygon/settings changed; nothing deleted');
}
if(!await eda.pcb_PrimitiveFill.delete(selected.map(f=>f.getState_PrimitiveId())))throw new Error('Fill deletion failed');
if(!await eda.pcb_Document.save())throw new Error('PCB save failed');
const remaining=(await eda.pcb_PrimitiveFill.getAll()).filter(f=>f.getState_Net()==='GND'&&[1,2].includes(Number(f.getState_Layer())));
if(remaining.length)throw new Error('GND fills still present; inspect saved export');
return JSON.stringify({deleted:selected.length,remainingGndFills:0,saved:true,scope:'Robot GND fills on layers1/2 only'});
