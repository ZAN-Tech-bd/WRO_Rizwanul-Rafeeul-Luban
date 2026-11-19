// READ-ONLY: compare live schematic pin/net membership and exported PCB/schematic netlists.
// Opens tabs for inspection; never Import Changes, setNetlist, or library writes.
const T={project:'ea32443fed8f1fd5989b3f8520da07a81d65c35e38ca7f144b04f7dc31730e1f',pcb:'8efe5b895e7646ea',page:'a013c67bfde245d3'};
if((await eda.dmt_Project.getCurrentProjectInfo())?.uuid!==T.project)throw new Error('Wrong project');
await eda.dmt_EditorControl.openDocument(T.pcb);
const pcbComponents=[];
for(const c of await eda.pcb_PrimitiveComponent.getAll()){
  pcbComponents.push({id:c.getState_PrimitiveId(),des:c.getState_Designator(),footprint:c.getState_Footprint(),
    pins:(await c.getAllPins()).map(q=>({number:q.getState_PadNumber(),net:q.getState_Net()}))});
}
let pcbNetlist,pcbNetlistError;
try{pcbNetlist=await eda.pcb_Net.getNetlist('EasyEDA');}catch(e){pcbNetlistError=String(e);}
await eda.dmt_EditorControl.openDocument(T.page);
if((await eda.dmt_SelectControl.getCurrentDocumentInfo())?.uuid!==T.page)throw new Error('Schematic not active');
const schComponents=[];
for(const c of await eda.sch_PrimitiveComponent.getAll()){
 schComponents.push({id:c.getState_PrimitiveId(),des:c.getState_Designator(),footprint:c.getState_Footprint(),
  attrs:(await eda.sch_PrimitiveAttribute.getAll(c.getState_PrimitiveId())).map(a=>({id:a.getState_PrimitiveId(),key:a.getState_Key(),value:a.getState_Value()})),
  pins:(await c.getAllPins()).map(q=>({number:q.getState_PinNumber(),x:q.getState_X(),y:q.getState_Y()}))});
}
const wires=(await eda.sch_PrimitiveWire.getAll()).map(w=>({id:w.getState_PrimitiveId(),net:w.getState_Net(),line:w.getState_Line()}));
const pinNets=[],ambiguous=[];
for(const c of schComponents)for(const pin of c.pins){
 const touching=wires.filter(w=>{const a=w.line.flat(3);return(a[0]===pin.x&&a[1]===pin.y)||(a[a.length-2]===pin.x&&a[a.length-1]===pin.y);});
 if(touching.length>1)ambiguous.push(c.des+'.'+pin.number);
 if(touching.length===1)pinNets.push({des:c.des,number:String(pin.number),net:touching[0].net});
}
let schNetlist,schNetlistError;
try{const f=await eda.sch_ManufactureData.getNetlistFile('hub_current_audit','EasyEDA');schNetlist=f?await f.text():undefined;}catch(e){schNetlistError=String(e);}
const ercDetailed=await eda.sch_Drc.check(true,true,true),ercPassed=await eda.sch_Drc.check();
return JSON.stringify({pcbComponents,schComponents,wires,pinNets,ambiguous,pcbNetlist,pcbNetlistError,schNetlist,schNetlistError,ercPassed,ercDetailed});
