// Hub v0.4 — full project export (READ-ONLY): getProjectFile('epro2') -> base64. Backup + persistence evidence:
// the archive is what EasyEDA actually serialized, unlike canvas getters (H-001: a visible footprint saved with 0 pads).
// Exports the SAVED state: save the documents before calling it. Decode with export_project.py; never trust the canvas alone.
// Run: python "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\hub\export_project.py" <tag>
const PROJECT = 'WRO_HUB';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const file = await eda.sys_FileManager.getProjectFile('WRO_HUB_export', undefined, 'epro2');
if (!file) return JSON.stringify({ abort: 'getProjectFile returned nothing' });
const bytes = new Uint8Array(await file.arrayBuffer());
let bin = '';
for (let i = 0; i < bytes.length; i += 0x8000) bin += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
return JSON.stringify({ name: file.name, size: bytes.length, b64: btoa(bin) });
