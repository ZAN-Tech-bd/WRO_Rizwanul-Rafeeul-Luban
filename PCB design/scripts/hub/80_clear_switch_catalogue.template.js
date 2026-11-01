// WRITE isolated recovery schematic ONLY: clear false VH-6 supplier/mechanical metadata on J3.
// Existing explicit custom footprint/Name/pin/wire mappings are preserved. No device-library API.
const T=__TARGET__,PLAN=__PLAN__;
const p=await eda.dmt_Project.getCurrentProjectInfo();
if(p?.uuid!==T.project||p?.friendlyName!==T.name)throw new Error('Wrong repair project');
if(document.visibilityState!=='visible')throw new Error('Keep repair editor visible');
await eda.dmt_EditorControl.openDocument(T.page);
const j3=(await eda.sch_PrimitiveComponent.getAll()).find(c=>c.getState_Designator()==='J3');
if(j3?.getState_PrimitiveId()!=='f26ea52ce64d9e40'||j3.getState_Footprint()?.uuid!==T.footprint)throw new Error('Wrong J3 instance');
const attrs=await eda.sch_PrimitiveAttribute.getAll(j3.getState_PrimitiveId());
for(const q of PLAN){const a=attrs.find(a=>a.getState_PrimitiveId()===q.id);
 if(!a||a.getState_Key()!==q.key)throw new Error('J3 attribute identity changed');
}
for(const q of PLAN)if(!await eda.sch_PrimitiveAttribute.modify(q.id,{value:q.value}))throw new Error('J3 metadata write failed');
if(!await eda.sch_Document.save())throw new Error('Schematic save failed');
const erc=await eda.sch_Drc.check(true,true,true);
return JSON.stringify({j3MetadataAttributesUpdated:PLAN.length,erc,deviceLibraryWrites:0,wireOrPinEdits:0});
