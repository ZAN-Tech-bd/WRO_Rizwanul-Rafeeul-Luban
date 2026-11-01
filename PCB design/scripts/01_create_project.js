// Step 1 — create the EasyEDA project for the Museum Hub Carrier board.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/01_create_project.js"
const projectUuid = await eda.dmt_Project.createProject(
  'Museum Hub Carrier v0.1',
  'MuseumHubCarrier-v0-1', // a-zA-Z0-9- only
  undefined, // personal space
  undefined, // root folder
  'WRO Smart Museum - ESP32 DevKitC 38-pin carrier: WS2812 rail (GPIO15), DHT11 x4 (14/16/17/27), MQ x4 (34/35/36/39 via dividers), passive buzzer (GPIO4), emergency button (GPIO21), microSD header (5/18/19/23), 5V barrel input. JLCPCB 2-layer prototype.'
);

// Wait for workspace sync, then read back to confirm
await new Promise((r) => setTimeout(r, 1500));
const brief = await eda.dmt_Project.getProjectInfo(projectUuid);

return JSON.stringify({ projectUuid, brief });
