// WRITES PCB1 in WRO_HUB only: J3 six-pad local override + own labels + local outline tab.
// All other component positions, mounting holes and copper are preserved. REAPPLY after schematic import.
// Native project-library creation is unavailable here. The legacy library is only a geometry carrier.
// Generated PLAN is tested by test_rocker_model.py; native custom footprint source is supplied separately.
// DISABLED: this editor rejects individual component-pad positions; partial move was restored by 49.
throw Error('Disabled: component-pad location overrides are unsupported. Open J3 in the footprint editor instead.');
const PLAN = __LIVE_PLAN__;
const MM = 1 / 0.0254;
if (document.visibilityState !== 'visible') throw Error('EasyEDA must be visible');
const project = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (project?.friendlyName !== 'WRO_HUB' || doc?.uuid !== 'ad35aed4d7610fd2' || doc.documentType !== 3)
  throw Error('Wrong project/document; no writes');
const ids = await eda.pcb_PrimitiveComponent.getAllPrimitiveId();
const components = await eda.pcb_PrimitiveComponent.get(ids);
const matches = components.filter(c => c.getState_Designator() === 'J3');
if (matches.length !== 1) throw Error('Expected one J3');
const component = matches[0];
const pins = await component.getAllPins();
if (pins.length !== 6 || PLAN.pads.some(w => !pins.some(p => String(p.getState_PadNumber()) === w.number && p.getState_Net() === w.net)))
  throw Error('J3 six-net identity mismatch; no writes');
const before = components.filter(c => c !== component).map(c => [c.getState_PrimitiveId(), c.getState_X(), c.getState_Y(), c.getState_Rotation()]);
// Check the ENTIRE flange/courtyard against live parts; not just the old carrier bbox.
const [l,b,r,t] = PLAN.courtyardBoundsMil;
for (const c of components.filter(c => c !== component)) {
  const bb = await eda.pcb_Primitive.getPrimitivesBBox([c.getState_PrimitiveId()]);
  if (bb && l < bb.maxX && r > bb.minX && b < bb.maxY && t > bb.minY)
    throw Error('Switch courtyard collides with ' + c.getState_Designator());
}
const initialSource = await eda.sys_FileManager.getDocumentSource();
const parse = source => source.trim().split(/\r?\n/).map(line => {
  const i = line.indexOf('||'); if (i < 0) throw Error('Unexpected source record');
  return [JSON.parse(line.slice(0,i)), JSON.parse(line.slice(i+2).replace(/\|\s*$/, ''))];
});
const initialRecords = parse(initialSource);
if (initialRecords.filter(([a,v]) => a.type === 'POLY' && v.layerId === 11).length !== 1)
  throw Error('Expected exactly one outline polygon; no writes');
await eda.pcb_PrimitiveComponent.modify(component.getState_PrimitiveId(), {
  x: PLAN.centerMil[0], y: PLAN.centerMil[1], rotation: 0, name: 'KCD4-203-B DIRECT',
  manufacturerId: 'KCD4-203-B', supplier: null, supplierId: null,
  otherProperty: { 'Direct mount': 'KCD4-203-B, DPDT ON-OFF-ON', 'Plated slots': '7.8 x 1.8 mm',
    'Physical fit': 'UNVERIFIED - real switch required', 'After import': 'Reapply 48_rocker_pcb.js',
    'Legacy carrier': 'B6P-VH library binding only; NOT the physical part' },
});
const moved = await eda.pcb_PrimitiveComponent.get(component.getState_PrimitiveId());
for (const pad of await moved.getAllPins()) {
  const want = PLAN.pads.find(p => p.number === String(pad.getState_PadNumber()));
  const a = pad.toAsync();
  a.setState_X(want.x); a.setState_Y(want.y); a.setState_Rotation(0);
  a.setState_Pad(['OVAL', 9 * MM, 3 * MM]);
  a.setState_Hole(['SLOT', 1.8 * MM, 7.8 * MM]);
  a.setState_HoleRotation(0); a.setState_Metallization(true);
  await a.done();
}
// Read the CURRENT source after pad overrides; never overwrite them with the earlier snapshot.
const records = parse(await eda.sys_FileManager.getDocumentSource());
const outline = records.find(([a,v]) => a.type === 'POLY' && v.layerId === 11)[1];
outline.path = [...PLAN.outlineMil[0], 'L', ...PLAN.outlineMil.slice(1).flat()];
const own = id => typeof id === 'string' && id.startsWith('wro_rocker_');
let updated = records.filter(([a]) => !own(a.id));
// Remove only obsolete free-standing switch-harness captions, never all board silkscreen.
const stale = new Set(['SWITCH', 'SWITCH J3', 'ROCKER SWITCH', '1 BAT+  2 V12  3 DC+', '4 BAT-  5 GND  6 DC-']);
updated = updated.filter(([a,v]) => !(a.type === 'STRING' && stale.has(v.text)));
const poly = (id, layer, bounds) => {
  const [x0,y0,x1,y1] = bounds;
  updated.push([{type:'POLY',id:'wro_rocker_'+id},{partitionId:'',groupId:0,netName:'',layerId:layer,
    width:0.2*MM,path:[x0,y0,'L',x1,y0,x1,y1,x0,y1,x0,y0],locked:false,zIndex:0,polyType:'NORMAL'}]);
};
poly('flange',3,PLAN.flangeBoundsMil);
poly('courtyard',14,PLAN.courtyardBoundsMil);
const source = updated.map(([a,v]) => JSON.stringify(a)+'||'+JSON.stringify(v)+'|').join('\n')+'\n';
if (!await eda.sys_FileManager.setDocumentSource(source)) throw Error('Native source update rejected');
const after = await eda.pcb_PrimitiveComponent.get(ids);
for (const [id,x,y,rot] of before) {
  const c=after.find(c=>c.getState_PrimitiveId()===id);
  if (!c || c.getState_X()!==x || c.getState_Y()!==y || c.getState_Rotation()!==rot)
    throw Error('Unexpected change to another component: '+id);
}
await eda.pcb_Document.save();
const actual = await eda.pcb_PrimitiveComponent.get(component.getState_PrimitiveId());
return { applied:true, otherComponentsUnchanged:before.length, component:actual.getState_Designator(),
  pads:(await actual.getAllPins()).map(p=>({number:p.getState_PadNumber(),net:p.getState_Net(),x:p.getState_X(),y:p.getState_Y(),
    pad:p.getState_Pad(),hole:p.getState_Hole(),holeRotation:p.getState_HoleRotation()})),
  note:'Local pad overrides: reapply after Import Changes. Physical fit not verified.' };
