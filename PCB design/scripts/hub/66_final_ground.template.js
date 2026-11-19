// Final user layout: add validated GND polygons, locally relocate three unused GND stitches,
// and correct one reversed polarity legend. NO component, signal-route, net, footprint or outline edits.
// Resumable, source-locked, 15-second batches. Save and full-export verification required after completion.
const CONFIG=__FINAL_GROUND_CONFIG__;
const fail=m=>{throw new Error(m);};
const project=await eda.dmt_Project.getCurrentProjectInfo(),doc=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(project?.friendlyName!=='WRO_HUB'||doc?.uuid!=='ad35aed4d7610fd2'||doc?.documentType!==3)fail('Wrong project/PCB');
if(document.visibilityState!=='visible')fail('Keep EasyEDA visible');
const near=(a,b)=>Math.abs(a-b)<.002;
const nearSize=(a,b)=>Math.abs(a-b)<.11;   // archive vs live-getter unit rounding (~0.05 mil), still << any real edit
const eq=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
const components=[];
for(const c of await eda.pcb_PrimitiveComponent.getAll()){
  const pads=(await c.getAllPins()).map(p=>({id:p.getState_PrimitiveId(),number:p.getState_PadNumber(),net:p.getState_Net(),x:p.getState_X(),y:p.getState_Y()})).sort((a,b)=>String(a.number).localeCompare(String(b.number)));
  components.push({id:c.getState_PrimitiveId(),designator:c.getState_Designator(),x:c.getState_X(),y:c.getState_Y(),rotation:c.getState_Rotation(),footprint:c.getState_Footprint(),pads});
}
components.sort((a,b)=>a.designator.localeCompare(b.designator));
if(!eq(components,CONFIG.components))fail('Component or pad geometry changed; re-plan');
const lines=(await eda.pcb_PrimitiveLine.getAll()).filter(p=>[1,2].includes(Number(p.getState_Layer())));
if(lines.length!==CONFIG.lines.length)fail('Track count changed');
const byLine=new Map(CONFIG.lines.map(p=>[p.id,p]));
for(const p of lines){
  const b=byLine.get(p.getState_PrimitiveId());
  if(!b||p.getState_Net()!==b.net||Number(p.getState_Layer())!==b.layer||
     !near(p.getState_StartX(),b.x1)||!near(p.getState_StartY(),b.y1)||!near(p.getState_EndX(),b.x2)||!near(p.getState_EndY(),b.y2)||!nearSize(p.getState_LineWidth(),b.width))fail('Track geometry changed');
}
const vias=await eda.pcb_PrimitiveVia.getAll();
if(vias.length!==CONFIG.vias.length)fail('Via count changed');
const byVia=new Map(CONFIG.vias.map(p=>[p.id,p]));
for(const p of vias){
  const b=byVia.get(p.getState_PrimitiveId()),move=CONFIG.viaMoves[p.getState_PrimitiveId()];
  if(!b||p.getState_Net()!==b.net||!nearSize(p.getState_HoleDiameter(),b.hole)||!nearSize(p.getState_Diameter(),b.diameter))fail('Via net/size changed');
  const at=(x,y)=>near(p.getState_X(),x)&&near(p.getState_Y(),y);
  if(!at(b.x,b.y)&&!(move&&at(move[0],move[1])))fail('Via moved since source snapshot');
}
const label=(await eda.pcb_PrimitiveString.getAll()).find(s=>s.getState_PrimitiveId()===CONFIG.legend.id);
if(!label||![CONFIG.legend.before,CONFIG.legend.after].includes(label.getState_Text()))fail('J5 legend identity changed');
const points=src=>{
  const a=(Array.isArray(src[0])?src[0]:src).filter(x=>typeof x==='number');
  const pts=[];for(let i=0;i<a.length;i+=2)pts.push([a[i],a[i+1]]);
  if(pts.length>1&&near(pts[0][0],pts[pts.length-1][0])&&near(pts[0][1],pts[pts.length-1][1]))pts.pop();
  return pts;
};
const segmentDistance=(p,a,b)=>{const x=b[0]-a[0],y=b[1]-a[1],den=x*x+y*y;
  const t=den?Math.max(0,Math.min(1,((p[0]-a[0])*x+(p[1]-a[1])*y)/den)):0;return Math.hypot(p[0]-a[0]-t*x,p[1]-a[1]-t*y);};
const sameOutline=(a,b)=>a.every(p=>b.some((q,i)=>segmentDistance(p,q,b[(i+1)%b.length])<.03))&&b.every(p=>a.some((q,i)=>segmentDistance(p,q,a[(i+1)%a.length])<.03));
const bounds=p=>[Math.min(...p.map(q=>q[0])),Math.min(...p.map(q=>q[1])),Math.max(...p.map(q=>q[0])),Math.max(...p.map(q=>q[1]))];
const plan=CONFIG.pieces.map(p=>({...p,points:points(p.source)}));
plan.forEach(p=>p.bounds=bounds(p.points));
const existing=await eda.pcb_PrimitiveFill.getAll();
const used=new Array(plan.length).fill(false);
for(const f of existing){
  if(f.getState_Net()!=='GND'||![1,2].includes(Number(f.getState_Layer())))fail('Unexpected existing fill');
  const pts=points(f.getState_ComplexPolygon().getSource()),bb=bounds(pts);
  const index=plan.findIndex((p,i)=>!used[i]&&p.layer===Number(f.getState_Layer())&&p.bounds.every((v,k)=>Math.abs(v-bb[k])<.05)&&sameOutline(pts,p.points));
  if(index<0)fail('Existing fill not in validated plan; do not stack copper');
  used[index]=true;
}
// All source/geometry checks above occur before any write.
for(const p of vias){const move=CONFIG.viaMoves[p.getState_PrimitiveId()];
  if(move&&(!near(p.getState_X(),move[0])||!near(p.getState_Y(),move[1]))){
    if(p.getState_Net()!=='GND')fail('Never relocate a non-GND via');
    if(!await eda.pcb_PrimitiveVia.modify(p.getState_PrimitiveId(),{x:move[0],y:move[1]}))fail('GND stitch relocation failed');
  }
}
if(label.getState_Text()!==CONFIG.legend.after)
  if(!await eda.pcb_PrimitiveString.modify(CONFIG.legend.id,{text:CONFIG.legend.after}))fail('Polarity legend update failed');
const started=Date.now();let created=0,remaining=0;
for(let i=0;i<plan.length;i++){
  if(used[i])continue;
  if(Date.now()-started>14500){remaining++;continue;}
  const polygon=eda.pcb_MathPolygon.createPolygon(plan[i].source);
  if(!polygon)fail('Invalid prevalidated polygon');
  if(!await eda.pcb_PrimitiveFill.create(plan[i].layer,polygon,'GND',0,.2,false))fail('Fill creation failed; inspect before retry');
  created++;
}
if(!await eda.pcb_Document.save())fail('Save failed');
return JSON.stringify({kept:used.filter(Boolean).length,created,remaining,totalPlanned:plan.length,done:remaining===0,
  relocatedGndStitches:Object.keys(CONFIG.viaMoves),polarityLegend:CONFIG.legend.after,saved:true});
