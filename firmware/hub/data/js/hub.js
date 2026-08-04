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
        K_ROBOT = "museum:robot", K_EMERG = "museum:emergency", K_SEED = "museum:seeded";
  function rd(k, f) { try { const v = localStorage.getItem(k); return v ? JSON.parse(v) : f; } catch (e) { return f; } }
  function wr(k, v) { localStorage.setItem(k, JSON.stringify(v)); window.dispatchEvent(new Event("museum:change")); }
  function uid() { return "V" + Date.now().toString(36) + Math.random().toString(36).slice(2, 5); }
  function seedIfEmpty() {
    if (localStorage.getItem(K_SEED)) return;
    wr(K_HEALTH, ARTIFACTS.map((a, i) => ({ id: a.id, temp: 24 + i, hum: 50 + i * 2, gas: 120 + i * 30 })));
    wr(K_ROBOT, { station: ARTIFACTS[0].id, status: "narrating" });
    wr(K_EMERG, false);
    localStorage.setItem(K_SEED, "1");
  }
  function mockState() {
    const reqs = rd(K_REQUESTS, []);
    return {
      robot: rd(K_ROBOT, { station: null, status: "idle" }),
      emergency: rd(K_EMERG, false),
      approved: reqs.filter((r) => r.status === "approved").length,
      pending: reqs.filter((r) => r.status === "pending").length,
      health: rd(K_HEALTH, []),
      requests: reqs
    };
  }

  // ---- cached state + subscribers (both backends use this) ----
  let cache = { robot: { station: null, status: "idle" }, emergency: false, approved: 0, pending: 0, health: [], requests: [] };
  const subs = new Set();
  function publish(s) { cache = s; subs.forEach((cb) => { try { cb(s); } catch (e) {} }); }

  async function pollReal() {
    try { const r = await fetch("/api/state"); const j = await r.json(); if (!j.requests) j.requests = []; publish(j); }
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
           requestCheckIn, getRequest, approve, deny, setEmergency, advanceRobot,
           getGallery, photoUrl, clearGallery };
})();
