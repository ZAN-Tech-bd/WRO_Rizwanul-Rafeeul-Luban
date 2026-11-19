// WRITE isolated recovery PCB: add validated current-geometry GND only, 14.5-second resumable batches.
const C=__CONFIG__;
__GUARD__
const plan=C.polygons.map(q=>({...q,points:points(q.source)}));plan.forEach(q=>q.bounds=bounds(q.points));
const used=new Array(plan.length).fill(false);
for(const f of await eda.pcb_PrimitiveFill.getAll()){
 if(f.getState_Net()!=='GND'||![1,2].includes(Number(f.getState_Layer())))fail('Unexpected fill');
 const pts=points(f.getState_ComplexPolygon().getSource()),bb=bounds(pts);
 const ix=plan.findIndex((q,i)=>!used[i]&&q.layer===Number(f.getState_Layer())&&q.bounds.every((v,k)=>Math.abs(v-bb[k])<.05)&&sameOutline(pts,q.points));
 if(ix<0)fail('Existing fill outside validated plan');used[ix]=true;
}
const start=Date.now();let created=0,remaining=0;
for(let i=0;i<plan.length;i++){
 if(used[i])continue;
 if(Date.now()-start>14500){remaining++;continue;}
 const poly=eda.pcb_MathPolygon.createPolygon(plan[i].source);if(!poly)fail('Invalid polygon');
 if(!await eda.pcb_PrimitiveFill.create(plan[i].layer,poly,'GND',0,.2,false))fail('Fill create refused; do not retry this run');created++;
}
if(!await eda.pcb_Document.save())fail('Save failed');
return JSON.stringify({kept:used.filter(Boolean).length,created,remaining,total:plan.length,done:remaining===0});
