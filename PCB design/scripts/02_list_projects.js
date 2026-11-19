// Step 1b — enumerate projects and find the board project.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/02_list_projects.js"
const uuids = await eda.dmt_Project.getAllProjectsUuid();
const briefs = [];
for (const u of uuids || []) {
  try {
    const b = await eda.dmt_Project.getProjectInfo(u);
    briefs.push({ uuid: u, friendlyName: b && b.friendlyName, folderUuid: b && b.folderUuid });
  } catch (e) {
    briefs.push({ uuid: u, error: String(e) });
  }
}
return JSON.stringify(briefs, null, 2);
