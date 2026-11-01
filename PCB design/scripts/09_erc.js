// Step 6 — run ERC on the schematic.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/09_erc.js"
function slim(v, depth) {
  if (depth <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 20).map((x) => slim(x, depth - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = slim(v[k], depth - 1);
    return o;
  }
  if (typeof v === 'string' && v.length > 200) return v.slice(0, 200) + '…';
  return v;
}
const r = await eda.sch_Drc.check();
return JSON.stringify(slim(r, 4), null, 1);
