// Robot v0.6 — full project export (READ-ONLY; copy of hub/51_export_project.js): getProjectFile('epro2') -> base64.
// Persistence evidence: the archive is what EasyEDA actually serialized, unlike canvas getters (hub H-001: component
// pad edits showed on the canvas but not in the saved project). Exports the SAVED state: save the documents first.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\51_export_project.js" > <out.json>
const PROJECT = 'WRO_Robot';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const file = await eda.sys_FileManager.getProjectFile('WRO_Robot_export', undefined, 'epro2');
if (!file) return JSON.stringify({ abort: 'getProjectFile returned nothing' });
const bytes = new Uint8Array(await file.arrayBuffer());
let bin = '';
for (let i = 0; i < bytes.length; i += 0x8000) bin += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
return JSON.stringify({ name: file.name, size: bytes.length, b64: btoa(bin) });
