// Hub v0.4 — step 3e: verify the wiring, run ERC, save. (v0.4: project gate added)
// (1) every pin that should be on a net has exactly one stub with that net name, and no stray wires;
// (2) EasyEDA's own net list; (3) ERC. Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/hub/10_check.js"
const _proj = await eda.dmt_Project.getCurrentProjectInfo();
if (!_proj || (_proj.friendlyName || _proj.name) !== 'WRO_HUB') return JSON.stringify({ abort: 'open project is not WRO_HUB' });
const res = {};
const wires = (await eda.sch_PrimitiveWire.getAll()) || [];
const at = new Map(); // "x,y" -> [net, ...]
for (const w of wires) {
  const f = (w.getState_Line() || []).flat(3);
  for (let i = 0; i + 1 < f.length; i += 2) {
    const k = Math.round(f[i]) + ',' + Math.round(f[i + 1]);
    if (!at.has(k)) at.set(k, []);
    at.get(k).push(w.getState_Net());
  }
}
res.wires = wires.length;
res.multiAtOnePoint = [...at.entries()].filter(([, v]) => v.length > 1).slice(0, 10);

const ids = await eda.sch_PrimitiveComponent.getAllPrimitiveId();
const comps = await eda.sch_PrimitiveComponent.get(ids);
const pinNets = []; // "DES.pin=NET"
let bare = 0;
for (const c of Array.isArray(comps) ? comps : [comps]) {
  const des = c.getState_Designator && c.getState_Designator();
  if (!des) continue;
  for (const p of (await c.getAllPins()) || []) {
    const k = Math.round(p.getState_X()) + ',' + Math.round(p.getState_Y());
    const nets = at.get(k);
    if (nets && nets.length) pinNets.push(des + '.' + p.getState_PinNumber() + '=' + nets[0]);
    else bare++;
  }
}
res.pinsWired = pinNets.length;
res.pinsBare = bare;
// group pins by net for a human-readable net list
const byNet = {};
for (const s of pinNets) {
  const [pin, net] = s.split('=');
  (byNet[net] = byNet[net] || []).push(pin);
}
res.netCount = Object.keys(byNet).length;
res.singlePinNets = Object.entries(byNet).filter(([, v]) => v.length < 2).map(([n, v]) => n + ':' + v.join(','));
res.netList = Object.entries(byNet).sort().map(([n, v]) => n + ' = ' + v.sort().join(' '));

try {
  const all = await eda.sch_Net.getAllNetsName();
  res.easyedaNetNames = Array.isArray(all) ? all.length : all;
} catch (e) { res.easyedaNetNames = 'ERR ' + e; }
try {
  res.erc = await eda.sch_Drc.check();
} catch (e) { res.erc = 'ERR ' + e; }
try { await eda.sch_Document.save(); res.saved = true; } catch (e) { res.saved = 'ERR ' + e; }
return JSON.stringify(res, null, 1);
