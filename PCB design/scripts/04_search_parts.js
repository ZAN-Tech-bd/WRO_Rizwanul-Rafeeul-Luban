// Step 2 — LCSC library search for dummy-board parts (read-only).
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/04_search_parts.js"
function slim(v, depth) {
  if (depth <= 0) return '[…]';
  if (Array.isArray(v)) return v.slice(0, 3).map((x) => slim(x, depth - 1));
  if (v && typeof v === 'object') {
    const o = {};
    for (const k of Object.keys(v)) o[k] = slim(v[k], depth - 1);
    return o;
  }
  return v;
}
const q = async (key) => {
  try {
    const r = await eda.lib_Device.search(key, undefined, undefined, undefined, 5, 1);
    return slim(r && (r.items ? r.items : r), 3);
  } catch (e) {
    return String(e);
  }
};
return JSON.stringify(
  {
    led_red_0603: await q('LED RED 0603'),
    res_1k_0603: await q('1kΩ 0603'),
    header_1x3: await q('Header Male 2.54mm 1x3'),
    terminal_2p_5mm: await q('screw terminal 5mm 2P'),
  },
  null,
  1
);
