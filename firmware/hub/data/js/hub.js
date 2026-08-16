// =====================================================================
//  hub.js  —  the only thing the web app talks to.
//
//  TWO backends, SAME function names (so the pages never change):
//   - On the real Hub (page is at http://192.168.4.1): uses fetch() to the
//     ESP32's /api endpoints.  <-- this is what runs at the venue.
//   - Anywhere else (localhost / file:// for local dev): a MOCK using
//     localStorage, so you can still test the UI without the ESP32.
//
//  It auto-detects: if location.hostname === "192.168.4.1" -> real Hub.
// =====================================================================

const Hub = (() => {
  const REAL = (location.hostname === "192.168.4.1");

  // ---- local-dev mock storage (only used when not on the Hub) ----
  const K_REQUESTS = "museum:requests", K_HEALTH = "museum:health",
        K_ROBOT = "museum:robot", K_EMERG = "museum:emergency", K_SEED = "museum:seeded",
        K_MUTED = "museum:muted";
  function rd(k, f) { try { const v = localStorage.getItem(k); return v ? JSON.parse(v) : f; } catch (e) { return f; } }
  function wr(k, v) { localStorage.setItem(k, JSON.stringify(v)); window.dispatchEvent(new Event("museum:change")); }
  function uid() { return "V" + Date.now().toString(36) + Math.random().toString(36).slice(2, 5); }
  function seedIfEmpty() {
    if (localStorage.getItem(K_SEED)) return;
    // Mirrors the real Hub: station 1 has both sensors, 2 only a DHT11,
    // 3 only an MQ -- so the dashboard's partial-row handling is exercised.
    wr(K_HEALTH, [
      { id: ARTIFACTS[0].id, temp: 27.0, hum: 62.0, gas: 2410 },
      { id: ARTIFACTS[1].id, temp: 29.5, hum: 58.0, gas: null },
      { id: ARTIFACTS[2].id, temp: null, hum: null, gas: 3305 }
    ]);
    wr(K_ROBOT, { station: ARTIFACTS[0].id, status: "narrating" });
    wr(K_EMERG, false);
    localStorage.setItem(K_SEED, "1");
  }
  // Off the Hub there are no real GPIOs to probe, so the pin scan is faked with
  // one answering pin and the real ESP32 pin facts (GPIO34-39 are input-only
  // and can only ever be analog) — enough to lay the admin table out locally.
  function mockSensors() {
    return [
      { pin: 14, dht: "ok",          ok: 7, tries: 7, inputOnly: false, station: "", temp: 27.0, hum: 62.0 },
      { pin: 16, dht: "no_response", ok: 0, tries: 7, inputOnly: false, station: "" },
      { pin: 17, dht: "no_response", ok: 0, tries: 7, inputOnly: false, station: "" },
      { pin: 27, dht: "checksum",    ok: 0, tries: 7, inputOnly: false, station: "" },
      { pin: 34, dht: "no_response", ok: 0, tries: 7, inputOnly: true,  station: "", adc: 1820, mv: 1466 },
      { pin: 35, dht: "no_response", ok: 0, tries: 7, inputOnly: true,  station: "", adc: 342,  mv: 275 },
      { pin: 36, dht: "no_response", ok: 0, tries: 7, inputOnly: true,  station: "", adc: 0,    mv: 0 },
      { pin: 39, dht: "no_response", ok: 0, tries: 7, inputOnly: true,  station: "", adc: 4095, mv: 3140 }
    ];
  }
  function mockState() {
    const reqs = rd(K_REQUESTS, []);
    const robot = rd(K_ROBOT, { station: null, status: "idle" });
    return {
      robot,
      emergency: rd(K_EMERG, false),
      muted: rd(K_MUTED, false),
      approved: reqs.filter((r) => r.status === "approved").length,
      pending: reqs.filter((r) => r.status === "pending").length,
      health: rd(K_HEALTH, []),
      sensors: mockSensors(),
      // No real camera off the Hub, so this mirrors the mock robot's own
      // station -- close enough to exercise admin.html's camera-derived
      // robot-status line, which "Advance robot (test)" otherwise no longer
      // touches now that it reads image.station instead of robot.station.
      image: { bytes: 4096, station: robot.station || "", seq: 1 },
      requests: reqs
    };
  }

  // ---- cached state + subscribers (both backends use this) ----
  // subscribe() hands this to its callback SYNCHRONOUSLY, before any real poll
  // has landed, so it needs every key a consumer might read off state.image —
  // admin.html's robot-status line does, and an absent key there would throw
  // on the very first render.
  let cache = { robot: { station: null, status: "idle" }, emergency: false, muted: false, approved: 0, pending: 0, health: [], sensors: [], requests: [], image: { bytes: 0, station: "", seq: 0 } };
  const subs = new Set();
  function publish(s) { cache = s; subs.forEach((cb) => { try { cb(s); } catch (e) {} }); }

  async function pollReal() {
    try {
      const r = await fetch("/api/state"); const j = await r.json();
      if (!j.requests) j.requests = [];
      if (!j.sensors)  j.sensors  = [];   // Hub built without the pin scan
      publish(j);
    }
    catch (e) { /* Hub briefly unreachable — keep last cache */ }
  }
  function pollMock() { publish(mockState()); }

  // ---------- public API (same names the pages already use) ----------
  function subscribe(cb) {
    subs.add(cb);
    cb(cache);
    if (REAL) { pollReal(); setInterval(pollReal, 1500); }
    else {
      seedIfEmpty(); pollMock();
      setInterval(pollMock, 1000);
      window.addEventListener("storage", pollMock);
      window.addEventListener("museum:change", pollMock);
    }
    return () => subs.delete(cb);
  }

  // sync getters read the cache (pages render from these)
  function getState() { return cache; }
  function listPending() { return (cache.requests || []).filter((r) => r.status === "pending"); }
  function countApproved() { return cache.approved; }
  function countPending() { return cache.pending; }

  // visitor check-in (async: returns a Promise<id>)
  async function requestCheckIn() {
    if (REAL) {
      const r = await fetch("/api/checkin", { method: "POST" });
      const j = await r.json(); return j.id;
    }
    const reqs = rd(K_REQUESTS, []); const id = uid();
    reqs.push({ id, ts: Date.now(), status: "pending" }); wr(K_REQUESTS, reqs); return id;
  }
  async function getRequest(id) {
    if (REAL) { const r = await fetch("/api/request?id=" + encodeURIComponent(id)); return await r.json(); }
    const x = rd(K_REQUESTS, []).find((r2) => r2.id === id);
    return { id, status: x ? x.status : "unknown" };
  }

  // commands: fire-and-forget on the real Hub (the next poll refreshes the UI)
  function fire(url) { if (REAL) fetch(url, { method: "POST" }).catch(() => {}); }
  function approve(id) {
    if (REAL) { fire("/api/approve?id=" + encodeURIComponent(id)); return; }
    const reqs = rd(K_REQUESTS, []); const r = reqs.find((x) => x.id === id); if (r) r.status = "approved"; wr(K_REQUESTS, reqs);
  }
  function deny(id) {
    if (REAL) { fire("/api/deny?id=" + encodeURIComponent(id)); return; }
    const reqs = rd(K_REQUESTS, []); const r = reqs.find((x) => x.id === id); if (r) r.status = "denied"; wr(K_REQUESTS, reqs);
  }
  function setEmergency(on) {
    if (REAL) { fire("/api/emergency?on=" + (on ? 1 : 0)); return; }
    wr(K_EMERG, !!on);
  }
  // Mute the robot's narration. The Hub latches this and re-broadcasts it over
  // ESP-NOW, so it survives a robot reboot -- see handleAudioMute() in the Hub
  // firmware. Fire-and-forget: the next /api/state poll confirms it.
  function setMute(on) {
    if (REAL) { fire("/api/audio/mute?on=" + (on ? 1 : 0)); return; }
    wr(K_MUTED, !!on);
  }
  // ---- ESP-CAM gallery -------------------------------------------------
  // The Hub keeps the newest N photos; /api/gallery lists them newest-first.
  // Off the Hub there is no camera, so the mock returns an empty gallery and
  // the admin page simply shows its "waiting for a photo" hint.
  async function getGallery() {
    if (!REAL) return { backend: "mock", count: 0, max: 0, newest: 0, photos: [] };
    const r = await fetch("/api/gallery");
    return await r.json();
  }
  // A seq is written once and never rewritten, so this URL is safe to cache —
  // that is what stops every thumbnail re-downloading on each poll.
  function photoUrl(seq) { return "/api/image?seq=" + encodeURIComponent(seq); }
  function clearGallery() { fire("/api/gallery/clear"); }

  function advanceRobot() { // admin "test" button
    if (REAL) { fire("/api/robot/advance"); return; }
    const cur = rd(K_ROBOT, { station: ARTIFACTS[0].id, status: "narrating" });
    const idx = ARTIFACTS.findIndex((a) => a.id === cur.station);
    const nx = ARTIFACTS[(idx + 1) % ARTIFACTS.length];
    wr(K_ROBOT, { station: nx.id, status: "moving" });
    setTimeout(() => wr(K_ROBOT, { station: nx.id, status: "narrating" }), 1500);
  }

  return { subscribe, getState, listPending, countApproved, countPending,
           requestCheckIn, getRequest, approve, deny, setEmergency, setMute,
           advanceRobot, getGallery, photoUrl, clearGallery };
})();
