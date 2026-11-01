// WRITES ONLY the user-opened project-local J3 footprint edit tab (doctype 4).
// Never changes a system library or writes footprint data into PCB1.
// Native drawing-based geometry is generated/tested by rocker_model.py.
// DISABLED: canvas readback succeeded, but the exported saved footprint contained zero pads.
throw Error('Disabled: footprint source save failed persistence verification. Do not replay this writer.');
const GEOMETRY = __GEOMETRY__;
const TARGET = 'ad35aed4d7610fd2_0cbe9290d583d889';
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== 'WRO_HUB' || doc?.documentType !== 4 || doc.uuid !== TARGET)
  throw Error('Open the project-local J3 footprint editor; wrong document, no writes');
if (document.visibilityState !== 'visible') throw Error('Keep the footprint editor visible');
const parse = s => s.trim().split(/\r?\n/).map(l => {
  const i=l.indexOf('||');
  return [JSON.parse(l.slice(0,i)),JSON.parse(l.slice(i+2).replace(/\|\s*$/,''))];
});
const source = await eda.sys_FileManager.getDocumentSource();
const original = parse(source);
const oldPads = original.filter(([a])=>a.type==='PAD');
if(oldPads.length!==6 || new Set(oldPads.map(([a,v])=>v.num)).size!==6)
  throw Error('Unexpected footprint pad identity');
if (!globalThis.__wroRockerEditorBefore) globalThis.__wroRockerEditorBefore = source;
const oldIds = new Map(oldPads.map(([a,v])=>[v.num,a.id]));
// Retain document setup and identity; remove ALL old connector geometry, not unrelated board geometry.
const geometryTypes=new Set(['PAD','POLY','FILL','LINE','ARC','CIRCLE','ELLIPSE','RECT','STRING']);
const out=original.filter(([a])=>!geometryTypes.has(a.type));
for(const [a,v] of out) if(a.type==='ATTR' && v.key==='Name') v.value='KCD4-203-B DIRECT';
for(const [a,v] of GEOMETRY){
  if(a.type==='PAD'){
    const old=oldPads.find(([a,p])=>p.num===v.num)[1];
    out.push([{type:'PAD',id:oldIds.get(v.num)}, {...old,...v,attrsMap:{}}]);
  } else if(a.type==='POLY') out.push([{type:'POLY',id:'wro_rocker_'+a.id},v]);
}
// A pin-one marker and part label make the top-view reference explicit without claiming I/II orientation.
const MM=1/0.0254;
const str=(id,text,x,y,size=0.85)=>out.push([{type:'STRING',id:'wro_rocker_'+id},{groupId:0,layerId:3,
  x:x*MM,y:y*MM,text,fontFamily:'default',fontSize:size*MM,strokeWidth:0.15*MM,
  bold:false,italic:false,origin:'CENTER_MIDDLE',angle:0,reverse:false,expansion:0,mirror:false,locked:false,zIndex:0}]);
str('name','KCD4-203-B',0,0,1.0);
for(const [a,pad] of GEOMETRY.filter(([a])=>a.type==='PAD'))
  str('pin_'+pad.num,pad.num,pad.centerX*0.0254,(Number(pad.num)<=3?10.8:-10.8));
const updated=out.map(([a,v])=>JSON.stringify(a)+'||'+JSON.stringify(v)+'|').join('\n')+'\n';
const apiResult=await eda.sys_FileManager.setDocumentSource(updated);
const read=parse(await eda.sys_FileManager.getDocumentSource());
const actualPads=read.filter(([a])=>a.type==='PAD');
if(actualPads.length!==6) throw Error('Readback does not contain exactly six pads');
for(const [a,p] of actualPads){
  const expected=GEOMETRY.find(([a,v])=>a.type==='PAD'&&v.num===p.num)?.[1];
  if(!expected || !p.plated || p.hole?.holeType!=='SLOT' || p.defaultPad?.padType!=='OVAL' ||
     Math.abs(p.centerX-expected.centerX)>0.001 || Math.abs(p.centerY-expected.centerY)>0.001 ||
     Math.abs(p.hole.width-expected.hole.width)>0.001 || Math.abs(p.hole.height-expected.hole.height)>0.001 ||
     p.relativeAngle!==90) throw Error('Footprint readback mismatch on pin '+p.num);
}
return {editorUpdated:true,apiResult,document:doc.uuid,oldPadIdsPreserved:true,
  pads:read.filter(([a])=>a.type==='PAD').map(([a,v])=>({id:a.id,number:v.num,xMm:v.centerX*0.0254,
    yMm:v.centerY*0.0254,hole:v.hole,pad:v.defaultPad,plated:v.plated})),
  next:'Save/apply this local footprint edit to PCB1 using the editor GUI; do not save to the system library.'};
