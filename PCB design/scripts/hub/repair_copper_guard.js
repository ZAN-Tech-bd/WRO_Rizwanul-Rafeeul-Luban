// Shared WRITE preflight. UUID, document, visibility, all component pad nets/poses, all tracks/vias.
const fail=m=>{throw new Error(m);},T=C.target;
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==T.project||d?.uuid!==T.pcb||d?.documentType!==3)fail('Wrong repair PCB');
if(document.visibilityState!=='visible')fail('Keep repair PCB visible');
const near=(a,b)=>Math.abs(a-b)<.002,nearSize=(a,b)=>Math.abs(a-b)<.11;
const components=[];
for(const c of await eda.pcb_PrimitiveComponent.getAll()){
 const pads=(await c.getAllPins()).map(p=>({id:p.getState_PrimitiveId(),number:p.getState_PadNumber(),net:p.getState_Net(),x:p.getState_X(),y:p.getState_Y()})).sort((a,b)=>String(a.number).localeCompare(String(b.number)));
 components.push({id:c.getState_PrimitiveId(),designator:c.getState_Designator(),x:c.getState_X(),y:c.getState_Y(),rotation:c.getState_Rotation(),footprint:c.getState_Footprint(),pads});
}
components.sort((a,b)=>a.designator.localeCompare(b.designator));
if(JSON.stringify(components)!==JSON.stringify(C.components))fail('Component/pad context changed');
const lines=(await eda.pcb_PrimitiveLine.getAll()).filter(q=>[1,2].includes(Number(q.getState_Layer())));
if(lines.length!==C.lines.length)fail('Track count changed');
const byLine=new Map(C.lines.map(q=>[q.id,q]));
for(const q of lines){const b=byLine.get(q.getState_PrimitiveId());
 const pos=(x1,y1,x2,y2)=>near(q.getState_StartX(),x1)&&near(q.getState_StartY(),y1)&&near(q.getState_EndX(),x2)&&near(q.getState_EndY(),y2);
 if(!b||q.getState_Net()!==b.net||Number(q.getState_Layer())!==b.layer||!nearSize(q.getState_LineWidth(),b.width)||(!pos(b.x1,b.y1,b.x2,b.y2)&&!pos(b.x2,b.y2,b.x1,b.y1)))fail('Track geometry changed');
}
const vias=await eda.pcb_PrimitiveVia.getAll();
if(vias.length!==C.vias.length)fail('Via count changed');
const byVia=new Map(C.vias.map(q=>[q.id,q]));
for(const q of vias){const b=byVia.get(q.getState_PrimitiveId());
 if(!b||q.getState_Net()!==b.net||!near(q.getState_X(),b.x)||!near(q.getState_Y(),b.y)||!nearSize(q.getState_HoleDiameter(),b.hole)||!nearSize(q.getState_Diameter(),b.diameter))fail('Via geometry changed');
}
const points=src=>{const a=(Array.isArray(src[0])?src[0]:src).filter(v=>typeof v==='number'),p=[];
 for(let i=0;i<a.length;i+=2)p.push([a[i],a[i+1]]);
 if(p.length>1&&near(p[0][0],p[p.length-1][0])&&near(p[0][1],p[p.length-1][1]))p.pop();return p;};
const segmentDistance=(p,a,b)=>{const x=b[0]-a[0],y=b[1]-a[1],den=x*x+y*y,t=den?Math.max(0,Math.min(1,((p[0]-a[0])*x+(p[1]-a[1])*y)/den)):0;return Math.hypot(p[0]-a[0]-t*x,p[1]-a[1]-t*y);};
const sameOutline=(a,b)=>a.every(p=>b.some((q,i)=>segmentDistance(p,q,b[(i+1)%b.length])<.03))&&b.every(p=>a.some((q,i)=>segmentDistance(p,q,a[(i+1)%a.length])<.03));
const bounds=p=>[Math.min(...p.map(q=>q[0])),Math.min(...p.map(q=>q[1])),Math.max(...p.map(q=>q[0])),Math.max(...p.map(q=>q[1]))];
