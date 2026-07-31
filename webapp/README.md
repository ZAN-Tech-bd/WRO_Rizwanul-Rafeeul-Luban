# Smart Museum — Web App (Phase 0)

The visitor + admin web app for the Smart Museum Guidance System (WRO 2026, Elementary).
Built first, per the team's decision, so the product is visible and testable before any hardware.

> **This is a complete, testable unit.** It uses a *mock* Hub (the browser's `localStorage`) so the
> visitor page and admin page talk to each other on one laptop. In Phase 6 the **same pages** will be
> served by the ESP32 Hub with **live** robot/health data — no UI rewrite needed.

## Files

| File | Job |
|------|-----|
| `index.html` | **Gate**: visitor scans QR (or Simulate) → asks admin for entry |
| `visitor.html` | **Visitor dashboard**: robot's station, queue note, emergency banner, the 7 artifacts |
| `admin.html` | **Admin dashboard**: visitor count, pending approvals, robot status, per-artifact health, emergency toggle |
| `css/style.css` | Museum theme (phone-first, Bangla/English) |
| `js/data.js` | The 7 artifacts + all on-screen text in Bangla & English |
| `js/hub.js` | The Hub (mock now → real ESP32 later) |
| `js/qr.js` | Offline QR reading from a camera photo |

## How to run it

**Best (for the two-tab demo):** start a tiny local server from this `webapp/` folder, then open the
pages in two browser tabs:

```powershell
# from the webapp folder:
python -m http.server 8000
```

Then open:
- Admin:   <http://localhost:8000/admin.html>
- Visitor: <http://localhost:8000/index.html>

**Quickest (no server):** just double-click `index.html`. Works for a single tab; the two-tab sync is
more reliable with the local server above.

## The flow (how to test it)

1. Open **`admin.html`** in one tab (this is the museum desk).
2. Open **`index.html`** in another tab (this is the visitor's phone).
3. On the visitor tab tap **Scan QR** (take a photo of any QR code) or **Simulate scan (demo)**.
   → The visitor tab shows "Waiting for admin approval…"
4. On the admin tab a **pending visitor** appears. Tap **Approve**.
5. The visitor tab auto-opens the **visitor dashboard** with the 7 artifacts.
6. On admin, try **Advance robot (test)** and **Start emergency (test)** — the visitor tab updates live.

## Offline QR (for the real demo, no internet)

Live camera streaming is blocked on plain HTTP (the ESP32 uses HTTP), so scanning works by **taking a
photo** of the QR code and decoding it offline with the free `jsQR` library:

1. Download `jsQR.js` (the plain, non-minified file is fine) from <https://github.com/cozmo/jsQR>.
2. Put it at `webapp/js/vendor/jsQR.js`.
3. That's it — `index.html` loads it automatically. If it's missing, visitors just use **Simulate**.

## How this becomes the real thing (Phase 6)

Right now `js/hub.js` stores everything in `localStorage`. Later, the ESP32 Hub serves these exact
files at `http://192.168.4.1` and the **same functions** in `hub.js` switch from `localStorage` to
`fetch("/api/...")`. The screens don't change. Example:

```js
// mock (now)
requestCheckIn()              // writes to localStorage
getState()                    // reads from localStorage

// real (Phase 6) — same names, different inside
requestCheckIn()  ->  fetch("/api/checkin", { method: "POST" })
getState()        ->  fetch("/api/state").then(r => r.json())
```

Live data that will flow in at Phase 6: robot station (over ESP-NOW), per-artifact
temperature/humidity/gas (DHT11 + MQ-135 at each artifact), visitor count, emergency state.

## Notes & decisions to make

- **7 artifacts** are defined in `js/data.js` (Jamdani, Nakshi Kantha, Liberation War, Terracotta,
  Ektara, Palki, Royal Bengal Tiger). Decide whether **all 7 are physical robot stops** or just
  dashboard content — 7 stops may be too many for a 5-minute demo in a 2×2 m booth (consider 3–4
  physical stops + all 7 on the dashboard).
- Health thresholds in `admin.html` (`classify()`) are placeholders — set real green/yellow/red
  values once the DHT11/MQ-135 readings are known.
- Bangla is the default language; tap **EN** / **বাংলা** to switch. Bengali renders natively in the
  phone browser — no font work needed.

## Troubleshooting

| Symptom | Fix |
|---|---|
| Visitor tab stays "waiting" | Did you Approve on the admin tab? Use the local server (not file://) for reliable two-tab sync. |
| Scan button does nothing | Add `jsQR.js` to `js/vendor/` or use **Simulate**. |
| Bangla looks like boxes | Use a modern phone browser; it has Bengali fonts built in. |
| Two tabs don't sync | Run via `python -m http.server` and open both via `http://localhost:8000/...`. |
