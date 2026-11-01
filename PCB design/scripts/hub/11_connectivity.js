// Hub v0.3 — geometric connectivity check (read-only). Wires join when an endpoint touches another wire
// anywhere (end or middle); a pin joins a wire it sits on. Reports any group carrying 2+ DIFFERENT net names
// (= short), groups with a repeated same name (= EasyEDA "multiple net names" warning), and stray unnamed wires.
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/11_connectivity.js"
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const W = wires.map((w) => {
  const f = (w.getState_Line() || []).flat(3).map((v) => Math.round(v));
  const pts = [];
  for (let i = 0; i + 1 < f.length; i += 2) pts.push([f[i], f[i + 1]]);
  return { id: w.getState_PrimitiveId(), net: w.getState_Net() || '', pts };
});
const onSeg = (p, a, b) => {
  const cross = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
  if (Math.abs(cross) > 0.5) return false;
  return p[0] >= Math.min(a[0], b[0]) && p[0] <= Math.max(a[0], b[0]) &&
         p[1] >= Math.min(a[1], b[1]) && p[1] <= Math.max(a[1], b[1]);
};
const onWire = (p, w) => {
  for (let i = 0; i + 1 < w.pts.length; i++) if (onSeg(p, w.pts[i], w.pts[i + 1])) return true;
  return w.pts.length === 1 && w.pts[0][0] === p[0] && w.pts[0][1] === p[1];
};
const parent = W.map((_, i) => i);
const find = (i) => (parent[i] === i ? i : (parent[i] = find(parent[i])));
const join = (a, b) => { parent[find(a)] = find(b); };
for (let i = 0; i < W.length; i++) {
  const ends = [W[i].pts[0], W[i].pts[W[i].pts.length - 1]];
  for (let j = 0; j < W.length; j++) {
    if (i === j) continue;
    if (ends.some((e) => e && onWire(e, W[j]))) join(i, j);
  }
}
// pins
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
const groupPins = {};
const pinGroups = {};
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  for (const p of (await c.getAllPins()) || []) {
    const pt = [Math.round(p.getState_X()), Math.round(p.getState_Y())];
    const tag = des + '.' + p.getState_PinNumber();
    const hits = new Set();
    W.forEach((w, i) => { if (onWire(pt, w)) hits.add(find(i)); });
    if (hits.size > 1) { const [first, ...rest] = [...hits]; rest.forEach((r) => join(r, first)); }
    pinGroups[tag] = hits.size ? [...hits][0] : null;
  }
}
const groups = {};
W.forEach((w, i) => {
  const g = find(i);
  (groups[g] = groups[g] || { names: [], wires: [], pins: [] }).names.push(w.net);
  groups[g].wires.push(w.id);
});
for (const [tag, g0] of Object.entries(pinGroups)) if (g0 !== null) groups[find(g0)].pins.push(tag);
const shorts = [], dupNames = [], unnamed = [];
for (const g of Object.values(groups)) {
  const named = g.names.filter(Boolean);
  const distinct = [...new Set(named)];
  if (distinct.length > 1) shorts.push({ names: distinct, pins: g.pins.slice(0, 12) });
  else if (named.length > 1) dupNames.push({ name: distinct[0], labels: named.length, pins: g.pins.slice(0, 6) });
  if (!named.length) unnamed.push({ wires: g.wires, pins: g.pins });
}
return JSON.stringify({ wires: W.length, groups: Object.keys(groups).length, shorts, dupNameGroups: dupNames.length,
  dupSample: dupNames.slice(0, 40), unnamed }, null, 0);
