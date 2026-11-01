// Offline execution of the real writer; the external editor is a strict stateful adapter.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const dir = __dirname;
const generated = fs.readFileSync(path.join(dir, '54_run.js'), 'utf8');
const config = JSON.parse(generated.match(/^const C = (.+);$/m)[1]);
const template = fs.readFileSync(path.join(dir, '54_fix_xt_footprints.js'), 'utf8');
const AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;

async function execute({ index = 0, changeConfig, changePad, wrongTab = false, staleReadback = false,
  quantizePad = false, saveOnly = false } = {}) {
  const c = structuredClone(config);
  c.footprints = [c.footprints[index]];
  const fp = c.footprints[0];
  const xy = index === 0 ? 141.7 : 98.4;
  const states = Object.entries(fp.pads).map(([id, want], n) => {
    want.xBefore = n ? xy : -xy;
    want.yBefore = 0;
    // Snapshot is from the fresh association-check archive, not rounded by this test.
    want.padBefore = index === 0 ? 216.5 : index === 1 ? 126 : 98.4;
    return { id, num: want.num, x: want.xBefore, y: 0,
      hole: ['ROUND', want.holeBefore], pad: ['ELLIPSE', want.padBefore, want.padBefore] };
  });
  if (changePad) changePad(states);
  if (changeConfig) changeConfig(c);
  if (saveOnly) {
    c.saveVerifiedOnly = true;
    for (const s of states) {
      s.hole = ['ROUND', fp.pads[s.id].holeAfter];
      s.pad = ['ELLIPSE', 137.8, 137.8];
    }
  }
  const editor = { documentType: 4, uuid: fp.uuid, tabId: fp.uuid + '@' + c.project };
  if (wrongTab) editor.tabId = 'unrelated-library';
  const writes = [];
  let saves = 0;
  const wrap = s => ({
    getState_PrimitiveId: () => s.id, getState_PadNumber: () => s.num,
    getState_X: () => s.x, getState_Y: () => s.y,
    getState_Hole: () => structuredClone(s.hole), getState_Pad: () => structuredClone(s.pad),
  });
  const eda = {
    dmt_Project: { getCurrentProjectInfo: async () => ({ uuid: c.project, friendlyName: 'WRO_Robot' }) },
    dmt_SelectControl: { getCurrentDocumentInfo: async () => editor },
    lib_Footprint: { openInEditor: async () => editor.tabId },
    dmt_EditorControl: { openDocument: async () => true },
    pcb_PrimitivePad: {
      getAll: async () => states.map(wrap),
      get: async id => wrap(states.find(s => s.id === id)),
      modify: async (id, props) => {
        writes.push({ id, props: structuredClone(props) });
        const s = states.find(s => s.id === id);
        if (!staleReadback) {
          if (props.hole) s.hole = structuredClone(props.hole);
          if (props.pad) s.pad = quantizePad ?
            [props.pad[0], Math.round(props.pad[1] * 10) / 10, Math.round(props.pad[2] * 10) / 10] :
            structuredClone(props.pad);
        }
        return wrap(s);
      },
    },
    pcb_Document: { save: async () => { saves++; return true; } },
  };
  let error;
  try {
    await new AsyncFunction('eda', 'document', template.replace('__CONFIG__', JSON.stringify(c)))(eda, { visibilityState: 'visible' });
  } catch (e) { error = e; }
  return { states, writes, saves, error };
}

test('builder target fields reach the XT60 editor as a finite 4 mm drill', async () => {
  const r = await execute();
  assert.ifError(r.error);
  assert.deepEqual(r.writes.map(w => w.props.hole), [['ROUND', 157.48], ['ROUND', 157.48]]);
  assert.equal(r.states[0].x, -141.7);
  assert.equal(r.states[1].x, 141.7);
});

test('XT30 target enlarges both copper pads and sets the 2.5 mm drill', async () => {
  const r = await execute({ index: 1 });
  assert.ifError(r.error);
  assert.deepEqual(r.writes.map(w => w.props), [
    { hole: ['ROUND', 98.425], pad: ['ELLIPSE', 137.795, 137.795] },
    { hole: ['ROUND', 98.425], pad: ['ELLIPSE', 137.795, 137.795] },
  ]);
});

test('wrong editor aborts before a pad write or save', async () => {
  const r = await execute({ wrongTab: true });
  assert.ok(r.error);
  assert.equal(r.writes.length, 0);
  assert.equal(r.saves, 0);
});

test('invalid target is rejected for the entire batch before any write', async () => {
  const r = await execute({ changeConfig: c => { delete Object.values(c.footprints[0].pads)[1].holeAfter; } });
  assert.ok(r.error, 'missing holeAfter must abort');
  assert.equal(r.writes.length, 0);
  assert.equal(r.saves, 0);
});

test('changed pad number aborts before modifying either pad', async () => {
  const r = await execute({ changePad: s => { s[1].num = '7'; } });
  assert.ok(r.error);
  assert.equal(r.writes.length, 0);
});

test('changed existing drill aborts before modifying either pad', async () => {
  const r = await execute({ changePad: s => { s[1].hole[1] = 12; } });
  assert.ok(r.error);
  assert.equal(r.writes.length, 0);
});

test('unchanged readback cannot be reported or saved as a successful correction', async () => {
  const r = await execute({ staleReadback: true });
  assert.ok(r.error);
  assert.equal(r.saves, 0);
});

test('the observed 0.1 mil pad-size serialization is accepted without relaxing hole or centre checks', async () => {
  const r = await execute({ index: 1, quantizePad: true });
  assert.ifError(r.error);
  assert.deepEqual(r.states[0].pad, ['ELLIPSE', 137.8, 137.8]);
  assert.equal(r.saves, 1);
});

test('verified-save-only mode persists the interrupted edit without repeating any pad modification', async () => {
  const r = await execute({ index: 1, saveOnly: true });
  assert.ifError(r.error);
  assert.equal(r.writes.length, 0);
  assert.equal(r.saves, 1);
});
