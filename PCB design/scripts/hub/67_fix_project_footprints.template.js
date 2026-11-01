// Authorized fabrication fixes. Edits actual project-library footprint PAD primitives and saves them.
// No component moves, pad creation/deletion, net edits or global library writes. IDs/centres remain fixed.
const PLAN=__FOOTPRINT_FIX_PLAN__;
const PROJECT='58cf0863ccb9b405a3019aadf223b9df2e72878780f06e4967d8bd8f9eeb441d';
const current=await eda.dmt_Project.getCurrentProjectInfo();
if(current?.friendlyName!=='WRO_HUB'||current.uuid!==PROJECT)throw new Error('Wrong project');
if(document.visibilityState!=='visible')throw new Error('Keep EasyEDA visible');
const results=[];
for(const [fp,entry] of Object.entries(PLAN)){
  await eda.lib_Footprint.openInEditor(fp,PROJECT);
  const doc=await eda.dmt_SelectControl.getCurrentDocumentInfo();
  if(doc?.documentType!==4||doc.uuid!==fp||doc.tabId!==fp+'@'+PROJECT)throw new Error('Not the project footprint editor');
  const pads=await eda.pcb_PrimitivePad.getAll();
  if(pads.length!==entry.pads.length)throw new Error('Footprint pad count changed');
  for(const want of entry.pads){
    const pad=pads.find(p=>p.getState_PrimitiveId()===want.id);
    if(!pad||String(pad.getState_PadNumber())!==String(want.before.num)||
       Math.abs(pad.getState_X()-want.before.centerX)>.1||Math.abs(pad.getState_Y()-want.before.centerY)>.1)
      throw new Error('Pad identity/centre mismatch');
  }
  for(const want of entry.pads){
    const n=want.after;
    const pad=[n.defaultPad.padType,n.defaultPad.width,n.defaultPad.height];
    if(n.defaultPad.padType==='RECT')pad.push(n.defaultPad.radius||0);
    const properties={pad};
    if(entry.designators[0]!=='Q1')properties.hole=['ROUND',n.hole.width];
    if(!await eda.pcb_PrimitivePad.modify(want.id,properties))throw new Error('Footprint pad modification failed');
  }
  if(!await eda.pcb_Document.save())throw new Error('Project footprint save failed');
  results.push({footprint:fp,designators:entry.designators,pads:entry.pads.length,saved:true});
}
await eda.dmt_EditorControl.openDocument('ad35aed4d7610fd2');
if(!await eda.pcb_Document.save())throw new Error('PCB save failed');
return JSON.stringify({results,verification:'REQUIRED: full-project export must prove persisted footprint PAD geometry and unchanged pad nets'});
