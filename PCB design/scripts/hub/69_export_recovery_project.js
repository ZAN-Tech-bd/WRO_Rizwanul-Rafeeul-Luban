// READ-ONLY export, pinned ONLY to the explicitly imported isolated recovery working copy.
const PROJECT='ea32443fed8f1fd5989b3f8520da07a81d65c35e38ca7f144b04f7dc31730e1f';
const p=await eda.dmt_Project.getCurrentProjectInfo();
if(p?.uuid!==PROJECT||p?.friendlyName!=='WRO_HUB_REPAIR_ISOLATED_20261027')throw new Error('Not the isolated recovery copy');
const f=await eda.sys_FileManager.getProjectFile('WRO_HUB_repaired_export',undefined,'epro2');
if(!f)throw new Error('Project export failed');
const bytes=new Uint8Array(await f.arrayBuffer());let s='';
for(let i=0;i<bytes.length;i+=0x8000)s+=String.fromCharCode.apply(null,bytes.subarray(i,i+0x8000));
return JSON.stringify({name:f.name,size:bytes.length,b64:btoa(s)});
