// Step 2b — pin exact canonical parts: red LED 0603 (C2286), 1k 0603 (C21190),
// plus targeted 1x3 header search. Also reads "JLCPCB Part Class" (Basic/Extended).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/05_pick_parts.js"
const pick = (item) => {
  if (!item) return null;
  return {
    name: item.name,
    lcsc: item.supplierId,
    manufacturer: item.manufacturer,
    manufacturerId: item.manufacturerId,
    uuid: item.uuid,
    libraryUuid: item.libraryUuid,
    symbol: item.symbolName,
    footprint: item.footprintName,
    jlcClass: item.otherProperty && item.otherProperty['JLCPCB Part Class'],
    stock: item.stock,
    price: item.price,
  };
};

let byIds = null;
try {
  const r = await eda.lib_Device.getByLcscIds(['C2286', 'C21190'], undefined, true);
  const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
  byIds = items.map(pick);
} catch (e) {
  byIds = String(e);
}

let headers = null;
try {
  const r = await eda.lib_Device.search('Pin Header 2.54mm 1x3P', undefined, undefined, undefined, 5, 1);
  const items = Array.isArray(r) ? r : r && r.items ? r.items : [r];
  headers = items.map(pick);
} catch (e) {
  headers = String(e);
}

return JSON.stringify({ byLcscIds: byIds, header1x3: headers }, null, 1);
