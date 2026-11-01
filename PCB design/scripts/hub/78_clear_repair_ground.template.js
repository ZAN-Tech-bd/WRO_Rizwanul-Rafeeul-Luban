// WRITE isolated recovery PCB: delete exactly checkpointed GND fills; all copper source guarded.
const C=__CONFIG__;
__GUARD__
const fills=await eda.pcb_PrimitiveFill.getAll();
if(!fills.length)return JSON.stringify({deleted:0,alreadyClear:true});
if(fills.length!==C.polygons.length)fail('Fill count changed');
const expected=new Map(C.polygons.map(q=>[q.id,q]));
for(const f of fills){const b=expected.get(f.getState_PrimitiveId());
 if(!b||f.getState_Net()!=='GND'||Number(f.getState_Layer())!==b.layer||!sameOutline(points(f.getState_ComplexPolygon().getSource()),points(b.source)))fail('Fill geometry changed');
}
if(!await eda.pcb_PrimitiveFill.delete(fills.map(q=>q.getState_PrimitiveId())))fail('Fill delete failed');
if(!await eda.pcb_Document.save())fail('Save failed');
return JSON.stringify({deleted:fills.length,componentChanges:0,trackChanges:0,viaChanges:0});
