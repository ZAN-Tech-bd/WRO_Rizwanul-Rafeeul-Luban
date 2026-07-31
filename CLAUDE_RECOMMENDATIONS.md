# Claude — Recommendations & Decision Log

Companion to [BUILD_PLAN.md](BUILD_PLAN.md). This file tracks **what we decided, what we
rejected, and what is still open**, so nothing gets re-argued later and nobody has to guess why
a choice was made.

- **Project:** Smart Museum Guidance System — WRO Future Innovators, **Elementary**
- **Log started:** 2026-07-31
- **Rule of thumb:** BUILD_PLAN.md = what to build. This file = why, and what's undecided.

---

## 1. Decisions locked (2026-07-31)

### D1 — LEGO AI Guard: **KEPT**, as a standalone side project ✅

Not integrated into the guide robot. Runs on its own at the booth as a separate demonstration
of the authentication idea.

**This is allowed.** Rule 5.1.2 does not ban on-market products — it says: *"If it makes sense
to use an on the market mechanism or robot in your solution, explain your choice."* So the
requirement is an **explanation**, not removal.

What that means in practice:

- **Prepare a 30-second answer** the kids can give when a judge asks "did you build this?"
  Something like: *"No — this is a ready-made AI kit. We used it to show what face
  authentication would look like in a real museum. Everything else on this table we built and
  coded ourselves."* Honest, confident, and it closes the question instead of opening it.
- **Physically separate it** on the booth table, with its own small label saying it's a
  ready-made kit. Judges reward honesty far more than they penalise a bought module; what
  costs points is a team that seems unclear about what they made.
- **Keep it out of the main run.** The 5-minute demo should be the guide robot + track +
  dashboard. The AI Guard is shown *after*, or during Q&A, or to booth visitors.
- **Restricted-area alert stays on RFID** in the main system (per the original priority
  matrix). The AI Guard demonstrates the *concept*; the RFID gate is the team's own
  implementation of it. That pairing is actually a good story: "here's the commercial way,
  here's how we did it ourselves for cheap."
- Because it's standalone, **feature 10 (confidence display) lives on the AI Guard itself** —
  no ESP-NOW link, no integration work. Good; that removes the work without removing the
  feature.

⚠️ *Open question — see Q1: I don't know which exact product this is, and it affects whether a
confidence number is even available.*

### D2 — ESP-NOW for board-to-board communication: **ADOPTED** ✅

Replaces Firebase in the critical path. No router, no internet, no credentials at the venue.
Technical notes in §3.

### D3 — 3.2" TFT: **KEPT, English text only** ✅

The TFT stays in the BOM (1810 tk). No Bangla rendering attempted on it — that avoids the
conjunct/matra shaping problem entirely.

**Bangla is not lost, it just moves to where it works properly:**

| Channel | Language | Why |
|---|---|---|
| Speaker (DFPlayer) | **Bangla + English** | Kids' own recorded voices |
| TFT display | **English only** | No shaping problem |
| **Phone dashboard** | **Bangla + English** | ⭐ Phone browsers have Bengali shaping built in — Bangla renders perfectly with zero effort |
| Printed booth & track labels | **Bangla + English** | Free |

The phone-browser point is worth noticing: it means the bilingual feature is fully intact and
visibly demonstrable, with no font engineering anywhere. Judges see Bangla on screen — just on
the phone, not the TFT.

---

## 2. Recommendation status — everything from the first review

| # | Recommendation | Status |
|---|---|---|
| R1 | Drop LEGO AI Guard | ❌ **Rejected** → kept standalone (D1) |
| R2 | Drop Firebase, use ESP-NOW + SoftAP | ✅ **Accepted** (D2) |
| R3 | Replace TFT with OLED | ❌ **Rejected** → TFT kept, English only (D3) |
| R4 | Drop face recognition from the *main* system | ✅ Effectively accepted — it's standalone now, RFID handles the restricted gate |
| R5 | Drop MQ-135 gas sensor | ❌ **Rejected** → kept per-artifact (D4) |
| R6 | Drop Bangla alphabet teaching module | ⬜ **Not yet discussed** |
| R7 | 2 drive motors instead of 4 | ⬜ **Not yet discussed** — saves ~1900 tk and improves line following |
| R8 | Separate buck converter for logic (brownout) | ⬜ **Not yet discussed** — strongest single reliability recommendation |
| R9 | Skip LM386 (DFPlayer has built-in 3W amp) | ⬜ **Not yet discussed** |
| R10 | 8/16 GB SD for DFPlayer, not 32 GB | ⬜ **Not yet discussed** |
| R11 | 1 kΩ series resistor on DFPlayer RX | ⬜ **Not yet discussed** |
| R12 | RFID tags on top of track, not underneath | ⬜ **Not yet discussed** |
| R13 | PVC chassis instead of 3D printing | ⬜ **Not yet discussed** — saves 3000 tk |
| R14 | ESP32-CAM serves photo locally, no Firebase Storage | ⬜ **Not yet discussed** |
| R15 | Museum visit + interview 2 non-family people (15 pts) | ⬜ **Not yet discussed** — cheapest points available |
| R16 | Write a slogan (10 pts) | ⬜ **Not yet discussed** |
| R17 | Team roles, shirts, all members speak (20 pts) | ⬜ **Not yet discussed** |
| R18 | Feature freeze 3 weeks before event | ⬜ **Not yet discussed** |
| R19 | Confirm season year / theme doc | ⬜ **Not yet discussed** — potentially blocking |

R5–R19 are still live. None conflict with D1–D3.

---

## 3. ESP-NOW — implementation notes (since D2 is locked)

**Topology.** One hub, several senders. The hub is the gate ESP32; it also runs the SoftAP that
phones connect to.

```
[Robot ESP32] ──┐
[Track ESP32] ──┼──► ESP-NOW ──► [Hub ESP32] ──► SoftAP "MuseumBot" ──► phones (QR)
[Station ESP32]─┘                                 serves the HTML dashboard
```

**The one gotcha that will cost you an evening:** ESP-NOW and SoftAP on the same board must run
on the **same WiFi channel**. Pin the hub's SoftAP to **channel 1**, and set channel 1
explicitly on every other board. If you skip this, messages arrive fine on the bench and stop
arriving the moment the hub starts its AP.

**Setup order for the kids:**

1. Run a "print my MAC address" sketch on each board once. Write the MAC on masking tape and
   stick it on the board. (Do this before anything is mounted — reading a MAC off a board
   buried in a chassis is miserable.)
2. Every board sends the *same* message struct. One struct, one meaning per field — much easier
   for kids than several message types:
   ```cpp
   typedef struct {
     uint8_t  fromWho;     // 1=robot 2=track 3=station 4=gate
     uint8_t  station;     // 0 = between stations
     uint8_t  state;       // 0=idle 1=driving 2=narrating 3=blocked 4=EMERGENCY
     uint8_t  language;    // 0=Bangla 1=English
     int16_t  value;       // temperature, visitor count, whatever that board reports
   } Msg;
   ```
3. Hub keeps one copy of the latest message from each sender and prints them into the web page.
4. **Emergency is a broadcast** — send to `FF:FF:FF:FF:FF:FF` so every board reacts without the
   hub relaying. Fewer hops, fewer failure points, and it works even if the hub is rebooting.

**Test to pass before moving on:** unplug the hub, plug it back in, and confirm the robot and
track keep working. Nothing except the dashboard should depend on the hub being alive.

---

## 4. TFT — implementation notes (since D3 is locked)

- **Use `Adafruit_ILI9341` + `Adafruit_GFX`, not `TFT_eSPI`.** TFT_eSPI is faster but requires
  hand-editing `User_Setup.h` inside the library folder — a classic place where a working
  project breaks after a library update, and very hard for a 10-year-old to debug. Adafruit's
  library takes pin numbers as normal arguments in the sketch. Slower, but the display is
  showing station names, not video.
- **No level shifter needed.** ILI9341 modules are 3.3 V logic and ESP32 is 3.3 V — they pair
  directly. (This is only a problem on 5 V Arduinos.) One less thing to build.
- **Backlight draws 50–100 mA.** Include it in the power budget; it's a real load on a battery.
- **Where should it go?** See Q2 — worth a conversation, because you only bought one.

---

## 5. Open questions for us

**Q1 — Which product is the "LEGO AI Guard", exactly?**
I couldn't identify it with confidence, and it matters: (a) does it output a numeric confidence
value, or just accept/reject? If it doesn't expose a number, feature 10 needs rewording. (b) Is
it actually LEGO-branded, or a LEGO-compatible third-party kit? That changes how the kids
describe it to judges. Send me the product name or a link.

**Q2 — Where does the single 3.2" TFT go: robot, or gate?**
You have one. My instinct is **gate**, for three reasons: the concept doc already describes "a
tablet/poster board at the gate" showing the visitor count; a 3.2" display is physically large
and power-hungry for a small line-following robot; and at the gate it's mains/bench powered and
faces the visitors. The robot would then get a ~350 tk OLED for station names.
Counter-argument: the TFT on the robot is more eye-catching during the moving demo.
**Not urgent, but decide before the chassis is cut.**

**Q3 — Which season are we actually targeting?**
Still the potentially blocking one (R19). All rulebooks in the folder are WRO 2025, and the
season theme document that defines "Area 1 / Area 3" isn't here. *Idea, Quality & Creativity* —
30 points, the largest line on the sheet — is scored on connection to the season theme.

**Q4 — How many exhibit stations on the track?**
Concept doc doesn't fix a number. I'd suggest **3**. Enough to prove the pattern, small enough
to fit a 2m × 2m booth with a real loop, and it keeps the 5-minute demo from running long.

**Q5 — Who is on the team?**
Names, ages (must be born 2013–2017), and who takes which role. Roles feed directly into *Team
Spirit* (20 pts) and the report's team page.

---

## 6. Change log

| Date | Change |
|---|---|
| 2026-07-31 | Initial review of concept doc + 3 rulebooks; BUILD_PLAN.md created |
| 2026-07-31 | D1 locked — AI Guard kept as standalone; BUILD_PLAN.md §1/§2 updated |
| 2026-07-31 | D2 locked — ESP-NOW adopted |
| 2026-07-31 | D3 locked — TFT kept, English only; BUILD_PLAN.md §4/§5 updated, BOM → ~18,200 tk |
| 2026-07-31 | **Season corrected to WRO 2026** ("Robots Meet Culture"); rule changes logged; build re-anchored |
| 2026-07-31 | D4 locked — MQ-135 kept at **every** artifact; per-station health to admin |
| 2026-07-31 | D5 locked — visitor/admin web app = one app, two views (routes) of one Hub source of truth |
| 2026-07-31 | BUILD_PLAN.md made canonical; `plans/smart-museum-plan.md` superseded |

---

## 7. 2026 season update (2026-07-31) — verified via official sources

Pulled the official **WRO 2026 Future Innovators General Rules** PDF and the 2026 season page.

- **Theme = "Robots Meet Culture"** (2025 was "The Future of Robots"). The concept's "Area 1 / Area 3"
  labels come from the **2026 Season Challenge (mission)**, whose three areas are:
  (1) protecting cultural heritage, (2) co-creation between humans & AI, (3) enhancing experiences
  with art & history. Our project targets **Area 1 + Area 3**; the AI Guard touches **Area 2**.
  This is an *even better* fit than 2025.
- **Rule changes 2025 → 2026** (General Rules):
  - Elementary = **born 2014–2018** (was 2013–2017). Confirm every member qualifies.
  - **NEW 6.5:** report **must disclose AI usage** (systems, purpose, extent). Judges may ask.
  - 7.11/9.3: certificates Bronze <50% / Silver 50–75% / Gold >75% (no Participation tier).
  - Scoring sheets **unchanged** (Elementary 200 = 70 / 65 / 65).
- **Action:** download the official 2026 General Rules + **Season Challenge** + Info for Teams from
  WROBd / [wro-association.org/competition/2026-season](https://wro-association.org/competition/2026-season/).
  The 2025 PDFs in the repo are outdated for theme/age/AI.

### D4 — MQ-135 gas sensor: **KEPT, one per artifact** ✅
Reverses R5. Each station reports temperature (DHT11) + humidity + gas (MQ-135) **separately** to the
admin, so any single change is detected and acted on (green/yellow/red threshold → alert). Strengthens
the Area 1 "protecting heritage" story. Trade-offs logged in BUILD_PLAN §4: 24–48 h burn-in, ~150 mA
heater each (own buck rail), relative-not-ppm readings. BOM rises ~1,000 tk vs the earlier cut.

### D5 — Visitor/Admin web app: one app, two views of one Hub ✅
Answers "how does data travel from the visitor's QR scan to the admin's device?":
- Both phones join the same Hub SoftAP ("MuseumBot", channel 1).
- **Visitor** opens `/` (QR → WiFi join → captive portal → `192.168.4.1`); read-only, ~2 s refresh.
- **Admin** opens `/admin` (PIN); sees per-artifact health table, counts, status, emergency reset.
- The **Hub is the single source of truth**; the two devices never talk directly. A visitor tap
  (`POST /api/help`) is stored on the Hub and appears on the admin's next refresh; an admin command
  (`POST /api/cmd`) is broadcast to nodes via ESP-NOW. Plain HTML + a little JS — kid-defensible.
- Full diagram + step-by-step: BUILD_PLAN.md §3.1.

### Open questions still live
- **Q1** LEGO AI Guard exact product (does it expose a numeric confidence?). Needed for the report.
- **Q2** Where the single 3.2" TFT goes (robot vs gate). Instinct: **gate**; decide before cutting chassis.
- **Q4** Number of stations (suggest **3**).
- **Q-new** Health-node topology: one ESP32 reading all artifacts, or one per station? Decide at Phase 6.
- **Q5** Team members: names, birth years (must be 2014–2018), roles (feeds Team Spirit 20 pts).

### D6 — The bridge is a DEDICATED Hub; nothing else hosts ✅ (2026-07-31)
Answers "which ESP32 should carry the QR request / host the app?":
- The **Hub/Gateway ESP32 at the gate** is the *only* node that runs SoftAP + the web server. Its only
  job is bridging, so it never interrupts another task.
- The **robot, CAM, track, health, and emergency nodes never host** — they only do small ESP-NOW
  packets (the CAM does one HTTP upload as a client). So the dashboard cannot slow the motors/LEDs/cam.
- **Host vs client:** the visitor phone and admin laptop are **browsers**, not hosts. Both open pages
  served by the Hub (`/` and `/admin.html`). "Admin app on laptop" = laptop is a client, not a server.
- All flows go through the Hub: `node →ESP-NOW→ Hub →HTTP→ screen` and back. See BUILD_PLAN §3.2.
- **QR entry:** gate poster = WiFi-join QR → phone joins "MuseumHub" → captive portal → Check-in →
  admin approves → visitor dashboard unlocks. Check-in count = attendance (IR beam now optional).
- Locked into BUILD_PLAN §3.2. Cost: +1 ESP32 for the dedicated Hub (or reuse it as the visitor counter).
