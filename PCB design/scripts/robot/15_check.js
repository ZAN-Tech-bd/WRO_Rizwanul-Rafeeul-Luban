// Robot carrier v0.4 — step P4f: No-Connect flags, geometric connectivity check, net list, ERC, save (WRITES NC flags).
// (1) flag every intentionally unused pin as No Connect (spec v0.4 §5 "No-connects");
// (2) union-find over wires + pins: a group holding 2+ different net names = SHORT; (3) pins neither wired nor NC;
// single-pin nets; stacked pins; (4) ERC (true = passed) and save. Feed the output to sch_audit.py.
// Run: node "D:\projects\WRO_Rizwanul-Rafeeul-Luban\tools\easyeda-bridge\eda.mjs" exec -f "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\15_check.js" > "D:\projects\WRO_Rizwanul-Rafeeul-Luban\PCB design\scripts\robot\data\sch_check_v04.json"
const NC = {
  // v0.6: J1–J4 are 2-pin vertical XT parts (D-030): no mounting tabs to flag
  U1A: ['3', '14'], // RST, GPIO46
  U1B: ['2', '3', '11', '12', '13', '14', '15', '16', '19', '20'], // 43, 44, 37, 36, 35, 0, 45, 48, 20, 19
  U2A: ['6', '8'], // SPK1, SPK2
  U2B: ['2', '3', '4', '5', '6', '8'], // USB-, USB+, ADKEY2, ADKEY1, IO2, IO1
  SW1: ['2', '4'], SW2: ['2', '4'], // diagonal pads 1/3 used (D-016)
};
// No timers: EasyEDA freezes its page timers while its window is hidden, so 'sleep' yields through cheap API calls.
const sleep = async (ms) => { const t = Date.now(); while (Date.now() - t < ms) await eda.dmt_Project.getCurrentProjectInfo(); };
// project gate + schematic tab: sch_* reads fail while the PCB tab is active
const PROJECT = 'WRO_Robot';
const proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!proj || (proj.friendlyName || proj.name) !== PROJECT) return JSON.stringify({ abort: 'open project is not ' + PROJECT });
const tree = await eda.dmt_EditorControl.getSplitScreenTree();
const tab = ((tree && tree.tabs) || []).find((t) => /schematic/i.test(t.title));
if (!tab) return JSON.stringify({ abort: 'no schematic tab open' });
await eda.dmt_EditorControl.activateDocument(tab.tabId);
await sleep(800);
const res = { ncSet: 0, errors: [] };
const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = (await eda.sch_PrimitiveComponent.get(ids)).filter((c) => c && c.getState_Designator && c.getState_Designator());
const pinsOf = {};
for (const c of comps) pinsOf[c.getState_Designator()] = (await c.getAllPins()) || [];
for (const [des, list] of Object.entries(NC)) {
  for (const p of pinsOf[des] || []) {
    if (!list.includes(String(p.getState_PinNumber()))) continue;
    try { if (!p.getState_NoConnected()) { const a = p.toAsync(); a.setState_NoConnected(true); await a.done(); } res.ncSet++; }
    catch (e) { res.errors.push(des + '.' + p.getState_PinNumber() + ' ' + e); }
  }
}
await sleep(1500);
// wires
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const W = [];
for (const w of wires) {
  const f = (w.getState_Line() || []).flat(3).map((v) => Math.round(v));
  const pts = [];
  for (let i = 0; i + 1 < f.length; i += 2) pts.push([f[i], f[i + 1]]);
  W.push({ net: w.getState_Net() || '', pts });
}
const onSeg = (p, a, b) => {
  const cross = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
  return Math.abs(cross) < 0.5 && p[0] >= Math.min(a[0], b[0]) && p[0] <= Math.max(a[0], b[0]) &&
    p[1] >= Math.min(a[1], b[1]) && p[1] <= Math.max(a[1], b[1]);
};
const onWire = (p, w) => w.pts.some((q, i) => i + 1 < w.pts.length && onSeg(p, q, w.pts[i + 1]));
const parent = W.map((_, i) => i);
const find = (i) => (parent[i] === i ? i : (parent[i] = find(parent[i])));
const join = (a, b) => { parent[find(a)] = find(b); };
for (let i = 0; i < W.length; i++) {
  const ends = [W[i].pts[0], W[i].pts[W[i].pts.length - 1]];
  for (let j = 0; j < W.length; j++) if (i !== j && ends.some((e) => onWire(e, W[j]))) join(i, j);
}
const groupPins = {};
const pinNet = {};
const bare = [];
let ncCount = 0;
const pinAt = {};
for (const [des, pins] of Object.entries(pinsOf)) {
  for (const p of pins) {
    const pt = [Math.round(p.getState_X()), Math.round(p.getState_Y())];
    const tag = des + '.' + p.getState_PinNumber();
    (pinAt[pt.join(',')] = pinAt[pt.join(',')] || []).push(tag);
    const hits = [];
    W.forEach((w, i) => { if (onWire(pt, w)) hits.push(i); });
    if (hits.length > 1) hits.slice(1).forEach((h) => join(h, hits[0]));
    if (hits.length) { pinNet[tag] = W[hits[0]].net; (groupPins[hits[0]] = groupPins[hits[0]] || []).push(tag); }
    else if (p.getState_NoConnected()) ncCount++;
    else bare.push(tag);
  }
}
const groups = {};
W.forEach((w, i) => { const g = find(i); (groups[g] = groups[g] || { names: new Set(), pins: [] }).names.add(w.net); });
for (const [i, pins] of Object.entries(groupPins)) groups[find(Number(i))].pins.push(...pins);
const shorts = Object.values(groups).filter((g) => g.names.size > 1).map((g) => [...g.names].join('+') + ' @ ' + g.pins.slice(0, 6).join(','));
const byNet = {};
for (const [tag, net] of Object.entries(pinNet)) (byNet[net] = byNet[net] || []).push(tag);
res.wires = W.length;
res.pinsWired = Object.keys(pinNet).length;
res.pinsNC = ncCount;
res.pinsBare = bare;
res.shorts = shorts;
res.stackedPins = Object.entries(pinAt).filter(([, v]) => v.length > 1).map(([k, v]) => k + ':' + v.join(','));
res.nets = Object.keys(byNet).length;
res.singlePinNets = Object.entries(byNet).filter(([, v]) => v.length < 2).map(([n, v]) => n + ':' + v.join(','));
res.netList = Object.entries(byNet).sort().map(([n, v]) => n + ' = ' + v.sort().join(' '));
try { res.erc = await eda.sch_Drc.check(); } catch (e) { res.erc = 'ERR ' + e; }
try { await eda.sch_Document.save(); res.saved = true; } catch (e) { res.saved = 'ERR ' + e; }
return JSON.stringify(res);
