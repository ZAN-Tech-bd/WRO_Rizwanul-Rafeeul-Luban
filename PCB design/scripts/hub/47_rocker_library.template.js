// WRITES only a NEW project-library footprint/device; never edits the original connector library.
// Generated payload: rocker_model.py -> data/rocker_footprint_source_20261026.efoo.
// Run the generated 47_rocker_library.js through the bridge; guard WRO_HUB and visible window.
const SOURCE = __FOOTPRINT_SOURCE__;
const NAME = 'KCD4-203-B_DIRECT_P10.5_R11_SLOT7.8x1.8';
if (document.visibilityState !== 'visible') throw Error('Bring EasyEDA to the foreground');
const project = await eda.dmt_Project.getCurrentProjectInfo();
if (project?.friendlyName !== 'WRO_HUB') throw Error('Wrong project; no writes');
const lib = await eda.lib_LibrariesList.getProjectLibraryUuid();
if (!lib || !['project', project.uuid].includes(lib)) throw Error('No verified Hub project library');
const cache = globalThis.__wroDirectRockerLibrary || {};
const footprint = cache.footprint || await eda.lib_Footprint.create(lib, NAME, [],
  'Drawing-based KCD4-203-B prototype: P10.5 R11; plated slots 7.8x1.8; physical fit NOT verified.');
if (!footprint) return { abort: 'Project-library footprint creation is unsupported in this workspace', lib };
globalThis.__wroDirectRockerLibrary = { lib, footprint };
const ok = await eda.lib_Footprint.updateDocumentSource(footprint, lib,
  SOURCE.replace('rocker-prototype', footprint));
if (!ok) return { abort: 'Footprint source update did not confirm success', lib, footprint, result: ok };
const device = cache.device || await eda.lib_Device.create(lib, NAME, [], {
  symbol: { uuid: '7c86b95a3e614d25a8008beade9630e8', libraryUuid: '0819f05c4eef4c71ace90d822a990e87' },
  footprint: { uuid: footprint, libraryUuid: lib },
}, 'Six-terminal DPDT centre-off switch; connection-block symbol preserves the verified pin map.', {
  name: 'KCD4-203-B', designator: 'J?', addIntoBom: true, addIntoPcb: true,
  manufacturerId: 'KCD4-203-B',
  otherProperty: { 'Physical fit': 'UNVERIFIED - actual switch required before fabrication',
    'Contact pitch': '10.5 mm', 'Pole spacing': '11 mm', 'Plated slots': '7.8 x 1.8 mm',
    'Outer flange': '34.5 x 25.5 mm', 'Description': 'DPDT ON-OFF-ON direct PCB mount, no harness' },
});
if (!device) return { abort: 'Custom device creation did not return an ID', lib, footprint };
globalThis.__wroDirectRockerLibrary = { lib, footprint, device };
return { lib, footprint, device, sourceUpdated: ok };
