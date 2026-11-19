// READ-ONLY Robot PCB: exact live fill create parameters; native polygon backup is stored separately.
const PROJECT='7c9ddded879e0beabea718339ea291c0ca140c2c8d71e5fdf6645641934d5864',PCB='643ce99542452c8f';
const p=await eda.dmt_Project.getCurrentProjectInfo(),d=await eda.dmt_SelectControl.getCurrentDocumentInfo();
if(p?.uuid!==PROJECT||p?.friendlyName!=='WRO_Robot'||d?.uuid!==PCB||d?.documentType!==3)throw new Error('Wrong Robot project/PCB');
const fills=[];
for(const f of await eda.pcb_PrimitiveFill.getAll()){
 const source=f.getState_ComplexPolygon().getSource();
 if(!Array.isArray(source))throw new Error('Missing fill polygon');
 fills.push({id:f.getState_PrimitiveId(),net:f.getState_Net(),layer:Number(f.getState_Layer()),source,
             fillMode:f.getState_FillMode(),lineWidth:f.getState_LineWidth(),lock:!!f.getState_PrimitiveLock()});
}
return JSON.stringify({project:PROJECT,name:p.friendlyName,pcb:PCB,fills});
