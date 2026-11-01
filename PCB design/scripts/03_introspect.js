// Step 1c — introspect the LIVE method surface of the editor-control and the
// open schematic document (read-only; safer + faster than guessing from docs).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/03_introspect.js"
const proto = (o) => Object.getOwnPropertyNames(Object.getPrototypeOf(o));
const out = {
  activeDocUuid: null,
  editorControl: proto(eda.dmt_EditorControl),
  schDocument: proto(eda.sch_Document),
  schNet: proto(eda.sch_Net),
  libDevice: proto(eda.lib_Device),
  libSymbol: proto(eda.lib_Symbol),
};
try {
  out.activeDocUuid = await eda.dmt_EditorControl.getActiveDocumentUuid();
} catch (e) {
  out.activeDocUuidErr = String(e);
}
return JSON.stringify(out, null, 2);
