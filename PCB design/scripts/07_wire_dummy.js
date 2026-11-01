// Step 4 — WIRE the dummy: net flags/labels on pins, then autoRouting() draws the wires.
// Nets: 5V (J1.1, R1.1, R2.1) · LED1_A/LED2_A (R→LED) · GND (J1.3, LED K) · SIG (J1.2)
// Run: node tools/easyeda-bridge/eda.mjs exec -f "PCB design/scripts/07_wire_dummy.js"
const log = { steps: [] };
const step = async (name, fn) => {
  try {
    const r = await fn();
    log.steps.push({ name, ok: true });
    return r;
  } catch (e) {
    log.steps.push({ name, ok: false, error: String(e) });
    return undefined;
  }
};

// 1) enumerate placed components
const ids = await step('getAllPrimitiveId', () => eda.sch_PrimitiveComponent.getAllPrimitiveId());
if (!ids || !ids.length) return JSON.stringify({ ...log, fail: 'no components found' }, null, 1);

const comps = await step('get', () => eda.sch_PrimitiveComponent.get(ids));
if (!comps || !comps.length) return JSON.stringify({ ...log, fail: 'get() returned nothing' }, null, 1);

// 2) map designator → pins (pinNumber, name, x, y)
const map = [];
for (const c of comps) {
  const d =
    (typeof c.getState_Designator === 'function' && c.getState_Designator()) ||
    (typeof c.getState_Name === 'function' && c.getState_Name()) ||
    '?';
  const pins = (await c.getAllPins()) || [];
  map.push({
    designator: d,
    pins: pins.map((p) => ({
      num: typeof p.getState_PinNumber === 'function' ? p.getState_PinNumber() : undefined,
      name:
        typeof p.getState_PinName === 'function'
          ? p.getState_PinName()
          : typeof p.getState_Name === 'function'
            ? p.getState_Name()
            : undefined,
      x: p.getState_X(),
      y: p.getState_Y(),
    })),
  });
}
log.components = map;

// NOTE (live finding): fresh placements are UNANNOTATED ("U?", "R?", "LED?") —
// detect by structure, not designator. A phantom 0-pin primitive also exists; skip it.
const real = map.filter((m) => m.pins.length > 0);
const headers = real.filter((m) => m.pins.length === 3);
const resistors = real
  .filter((m) => /^R/.test(String(m.designator)))
  .sort((a, b) => a.pins[0].y - b.pins[0].y); // y=400 chain1, y=700 chain2
const leds = real
  .filter((m) => /^(LED|D)/.test(String(m.designator)))
  .sort((a, b) => a.pins[0].y - b.pins[0].y);
if (!headers.length || resistors.length < 2 || leds.length < 2)
  return JSON.stringify({ ...log, fail: 'component roles not detected (see components list)' }, null, 1);

// 3) net plan — [x, y, net] points
const points = [];
const hdrPins = headers[0].pins.sort((a, b) => (a.num > b.num ? 1 : -1));
points.push({ ...hdrPins[0], net: '5V' });
points.push({ ...hdrPins[1], net: 'SIG' });
points.push({ ...hdrPins[2], net: 'GND', flag: 'Ground' });

for (let i = 0; i < 2; i++) {
  const r = resistors[i].pins.sort((a, b) => a.x - b.x);
  points.push({ ...r[0], net: '5V' });
  points.push({ ...r[1], net: `LED${i + 1}_A` });

  const led = leds[i].pins;
  const anode =
    led.find((p) => /^(A|AN|ANODE|1)$/i.test(String(p.name ?? p.num))) || led[0];
  const cathode =
    led.find((p) => /^(K|C|CATHODE|2)$/i.test(String(p.name ?? p.num))) || led[1];
  points.push({ ...anode, net: `LED${i + 1}_A` });
  points.push({ ...cathode, net: 'GND', flag: 'Ground' });
}

// 4) place net flags (GND) / labels (everything else)
for (const pt of points) {
  if (pt.flag) {
    await step(`netflag ${pt.net} @${Math.round(pt.x)},${Math.round(pt.y)}`, () =>
      eda.sch_PrimitiveComponent.createNetFlag(pt.flag, pt.net, pt.x, pt.y)
    );
  } else {
    await step(`label ${pt.net} @${Math.round(pt.x)},${Math.round(pt.y)}`, () =>
      eda.sch_PrimitiveAttribute.createNetLabel(pt.x, pt.y, pt.net)
    );
  }
}

// 5) auto-route wires between same-net points, then save
await step('autoRouting', () => eda.sch_Document.autoRouting());
await new Promise((r) => setTimeout(r, 800));
await step('save', () => eda.sch_Document.save());

// 6) verify nets
const nets = await step('getAllNetsName', () => eda.sch_Net.getAllNetsName());
log.nets = nets;

return JSON.stringify(log, null, 1);
