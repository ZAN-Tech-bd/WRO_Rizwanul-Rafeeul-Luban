#!/usr/bin/env node
/**
 * EasyEDA Pro bridge helper for Zoo Code / any terminal agent.
 *
 * Talks to the official easyeda/easyeda-api-skill bridge server
 * (https://github.com/easyeda/easyeda-api-skill), which relays JS to the
 * Run API Gateway extension inside EasyEDA Pro.
 *
 * Usage:
 *   node tools/easyeda-bridge/eda.mjs health
 *   node tools/easyeda-bridge/eda.mjs exec "eda.sys_GetAppVersion()"
 *   node tools/easyeda-bridge/eda.mjs exec -f script.js
 *
 * API reference: D:\projects\easyeda-api-skill\references\  (+ SKILL.md, guide\)
 */

const PORT_START = 49620;
const PORT_END = 49629;
const TIMEOUT_MS = 30_000;

async function findBridge() {
  for (let port = PORT_START; port <= PORT_END; port++) {
    try {
      const res = await fetch(`http://127.0.0.1:${port}/health`, {
        signal: AbortSignal.timeout(1500),
      });
      if (!res.ok) continue;
      const body = await res.json();
      if (body && body.service === 'easyeda-bridge') {
        return { port, health: body };
      }
    } catch {
      /* try next port */
    }
  }
  return null;
}

async function main() {
  const [cmd, ...rest] = process.argv.slice(2);

  const bridge = await findBridge();
  if (cmd !== 'health' && !bridge) {
    console.error(
      `ERROR: EasyEDA bridge not found on ports ${PORT_START}-${PORT_END}.\n` +
        `Start it with:  cd /d D:\\projects\\easyeda-api-skill && npm run server`
    );
    process.exit(2);
  }

  switch (cmd) {
    case 'health': {
      if (!bridge) {
        console.error('Bridge NOT running.');
        process.exit(2);
      }
      console.log(`bridge: http://127.0.0.1:${bridge.port}`);
      console.log(JSON.stringify(bridge.health, null, 2));
      if (!bridge.health.edaConnected) {
        console.error(
          'WARNING: EasyEDA Pro is not connected. Open EasyEDA Pro and click API Gateway -> Reconnect.'
        );
        process.exit(1);
      }
      return;
    }

    case 'exec': {
      let code = '';
      if (rest[0] === '-f') {
        const file = rest[1];
        if (!file) {
          console.error('Usage: eda.mjs exec -f <script.js>');
          process.exit(2);
        }
        code = (await import('node:fs')).readFileSync(file, 'utf8');
      } else {
        code = rest.join(' ');
      }
      if (!code.trim()) {
        console.error('Usage: eda.mjs exec "<javascript using the eda.* API>"');
        process.exit(2);
      }

      const res = await fetch(`http://127.0.0.1:${bridge.port}/execute`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code }),
        signal: AbortSignal.timeout(TIMEOUT_MS),
      });
      const text = await res.text();
      try {
        const body = JSON.parse(text);
        if (body.error) {
          console.error('EDA ERROR:', typeof body.error === 'string' ? body.error : JSON.stringify(body.error, null, 2));
          process.exit(1);
        }
        console.log(JSON.stringify(body.result ?? body, null, 2));
      } catch {
        console.log(text);
      }
      return;
    }

    default:
      console.log(
        'EasyEDA bridge helper\n' +
          '  node tools/easyeda-bridge/eda.mjs health          # bridge + EDA connection status\n' +
          '  node tools/easyeda-bridge/eda.mjs exec "<js>"     # run eda.* JS inside EasyEDA Pro\n' +
          '  node tools/easyeda-bridge/eda.mjs exec -f file.js # run JS from a file'
      );
      if (bridge) {
        console.log(`\nbridge: http://127.0.0.1:${bridge.port}  edaConnected=${bridge.health.edaConnected}`);
      }
  }
}

main()
  .catch((e) => {
    console.error('ERROR:', e.message);
    process.exitCode = 1;
  })
  .finally(() => {
    // Exit explicitly so the pending AbortSignal timers can't trip a
    // libuv assertion (0xC0000409) on Windows during teardown.
    setTimeout(() => process.exit(process.exitCode || 0), 50).unref();
  });
