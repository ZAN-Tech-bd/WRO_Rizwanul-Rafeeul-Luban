#!/usr/bin/env node
// Local batch driver for the guarded GND fill writer: re-POSTs the SAME source-locked
// script until it reports done:true. Aborts on any error (inspect, never blind-retry).
// The writer itself time-boxes creation to 14.5 s and saves every run, so each iteration
// is a safe checkpoint. Max 60 iterations, 3 s settle between runs.
import { readFileSync } from 'node:fs';

const code = readFileSync(new URL('./57_final_ground.js', import.meta.url), 'utf8');
const find = async () => {
  for (let port = 49620; port <= 49629; port++) {
    try {
      const r = await fetch(`http://127.0.0.1:${port}/health`, { signal: AbortSignal.timeout(1500) });
      if (r.ok && (await r.json()).service === 'easyeda-bridge') return port;
    } catch { /* next */ }
  }
  throw new Error('bridge not found');
};
const port = await find();
for (let i = 1; i <= 60; i++) {
  const res = await fetch(`http://127.0.0.1:${port}/execute`, {
    method: 'POST', headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ code }), signal: AbortSignal.timeout(60000) });
  const body = await res.json();
  if (body.error) { console.error('EDA ERROR:', body.error); process.exit(1); }
  const out = typeof body.result === 'string' ? JSON.parse(body.result) : body.result;
  console.log(JSON.stringify({ run: i, ...out }));
  if (out?.done) { console.log('ALL FILLS PLACED'); process.exit(0); }
  await new Promise(r => setTimeout(r, 3000));
}
console.error('iteration cap reached without done:true');
process.exit(1);
