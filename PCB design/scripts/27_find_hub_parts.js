// Diagnose — where are the WRO_HUB schematic parts? Tabs, pages, component count, bounds.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/27_find_hub_parts.js"
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const out = {};

// all editor tabs (both split screens if any)
try {
  const tree = await eda.dmt_EditorControl.getSplitScreenTree();
  out.tabs = (tree.tabs || []).map((t) => t.title);
} catch (e) {
  out.tabsErr = String(e);
}

// current schematic + its pages
try {
  const si = await eda.dmt_Schematic.getCurrentSchematicInfo();
  out.schematic = { uuid: si && si.uuid, name: si && si.name };
  out.pages = (si && si.page ? si.page : []).map((p) => ({ uuid: p.uuid, name: p.name }));
} catch (e) {
  out.schErr = String(e);
}

try {
  const pi = await eda.dmt_Schematic.getCurrentSchematicPageInfo();
  out.currentPage = { uuid: pi && pi.uuid, name: pi && pi.name };
} catch (e) {
  out.pageErr = String(e);
}

// components on the ACTIVE page: count + designators + bounding box
try {
  const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
  out.compCount = (ids || []).length;
  const comps = await eda.sch_PrimitiveComponent.get(ids || []);
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  const des = [];
  for (const c of comps) {
    const d = (typeof c.getState_Designator === 'function' && c.getState_Designator()) || '?';
    const x = typeof c.getState_X === 'function' ? c.getState_X() : NaN;
    const y = typeof c.getState_Y === 'function' ? c.getState_Y() : NaN;
    if (!Number.isNaN(x)) {
      if (x < minX) minX = x;
      if (x > maxX) maxX = x;
    }
    if (!Number.isNaN(y)) {
      if (y < minY) minY = y;
      if (y > maxY) maxY = y;
    }
    des.push(d);
  }
  out.bounds = {
    minX: Number.isFinite(minX) ? minX : null,
    minY: Number.isFinite(minY) ? minY : null,
    maxX: Number.isFinite(maxX) ? maxX : null,
    maxY: Number.isFinite(maxY) ? maxY : null,
  };
  out.desCount = des.length;
  out.designatorsSample = des.slice(0, 40);
} catch (e) {
  out.compErr = String(e);
}

// nets
try {
  out.nets = (await eda.sch_Net.getAllNetsName()) || [];
} catch (e) {
  out.netErr = String(e);
}

return JSON.stringify(out, null, 1);
