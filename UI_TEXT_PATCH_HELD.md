# Smart Museum — Bangla/English UI patch (HELD, not applied)

Prepared 2026-08-04 while another Claude session was mid-edit on `webapp/admin.html`,
`webapp/js/hub.js`, `webapp/js/data.js` and `webapp/css/style.css` (gallery feature).
**No project file was changed by this patch.** Apply after that session lands.

Every block below is find → replace. Anchors are short and unique, so they survive
line drift. Verify each `FIND` still matches before applying — the gallery session may
have moved things.

---

## A. `webapp/js/data.js` — corrections to existing copy

### A1. Spelling: "historical" is misspelt

FIND
```
বাংলাদেশের বিখ্যাত হাতে বোনা কাপড়, যা তার সুন্দর নকশার জন্য পরিচিত।
```
REPLACE
```
বাংলাদেশের বিশ্বখ্যাত হাতে বোনা কাপড়, সুন্দর নকশার জন্য যার খ্যাতি।
```
*(also removes the anglicism "যা তার … জন্য পরিচিত" = literal "known for its")*

FIND
```
১৯৭১ সালের মুক্তিযুদ্ধের ইউনিফর্ম, পদক, মানচিত্র ও ঐতিতাসিক ছবির মতো নিদর্শন।
```
REPLACE
```
১৯৭১ সালের মুক্তিযুদ্ধের ইউনিফর্ম, পদক, মানচিত্র ও ঐতিহাসিক ছবির মতো নিদর্শন।
```
`ঐতিতাসিক` is not a word. Correct spelling: `ঐতিহাসিক`.

### A2. Ektara described as the wrong kind of instrument

FIND
```
বাউল সঙ্গীতশিল্পীদের ব্যবহৃত একটানা বাদ্যযন্ত্র।
```
REPLACE
```
বাউল সঙ্গীতশিল্পীদের ব্যবহৃত এক তারের বাদ্যযন্ত্র।
```
`একটানা` means "continuous / non-stop". The English says "one-string", and the
one string is the whole point of an ektara — `এক তারের`.

### A3. "যান" twice in one sentence (palki)

FIND
```
আধুনিক যানের আগে গুরুত্বপূর্ণ মানুষদের বহন করতে ব্যবহৃত সজ্জিত যান।
```
REPLACE
```
আধুনিক যানবাহনের আগে গুরুত্বপূর্ণ মানুষদের বহন করতে ব্যবহৃত সাজানো বাহন।
```

### A4. Tagline is transliterated English, not Bangla

FIND
```
  tagline:      { en: "Robots Meet Culture",     bn: "রোবট মিটস কালচার" },
```
REPLACE
```
  tagline:      { en: "Robots Meet Culture",     bn: "রোবট ও সংস্কৃতির মেলবন্ধন" },
```

### A5. "Visitor" → proper Bangla, consistently (3 keys)

FIND
```
  gateTitle:    { en: "Visitor Check-in",        bn: "ভিজিটর চেক-ইন" },
```
REPLACE
```
  gateTitle:    { en: "Visitor Check-in",        bn: "দর্শনার্থী চেক-ইন" },
```

FIND
```
  visitorBadge: { en: "Visitor",                 bn: "ভিজিটর" },
```
REPLACE
```
  visitorBadge: { en: "Visitor",                 bn: "দর্শনার্থী" },
```

FIND
```
  visitorCount: { en: "Visitors inside",         bn: "ভেতরে ভিজিটর" },
```
REPLACE
```
  visitorCount: { en: "Visitors inside",         bn: "দর্শনার্থী ভেতরে" },
```

### A6. Emergency banner — the one line that must be unambiguous

FIND
```
  emergencyMsg: { en: "EMERGENCY — follow the robot to the nearest exit.", bn: "জরুরি অবস্থা — নিকটতম এক্সিটে যেতে রোবটের সাথে চলুন।" },
```
REPLACE
```
  emergencyMsg: { en: "EMERGENCY — follow the robot to the nearest exit.", bn: "জরুরি অবস্থা — রোবটকে অনুসরণ করে নিকটতম বেরোনোর পথে যান।" },
```
Drops the transliterated `এক্সিট` and turns it into an instruction, matching the English imperative.

### A7. Health table: "heat" → "temperature", "signal" → "danger"

FIND
```
  thTemp:       { en: "Temp °C",                 bn: "তাপ °C" },
```
REPLACE
```
  thTemp:       { en: "Temp °C",                 bn: "তাপমাত্রা °C" },
```

FIND
```
  alert:        { en: "Alert",                   bn: "সংকেত" },
```
REPLACE
```
  alert:        { en: "Alert",                   bn: "বিপদ!" },
```
`সংকেত` = "signal", which tells a judge nothing. The red pill should read `বিপদ`.

### A8. New keys — append before the closing `};` of `I18N`

These names avoid the gallery session's `camera*` keys. Add a comma to the line above first.

```js
  advanceRobot: { en: "▶ Advance robot (test)",  bn: "▶ রোবট পরের স্টেশনে (পরীক্ষা)" },
  healthNote:   { en: "Readings arrive live from each artifact's DHT11 + MQ-135 node over ESP-NOW.",
                  bn: "প্রতিটি নিদর্শনের DHT11 + MQ-135 নোড থেকে ESP-NOW-এ সরাসরি তথ্য আসে।" },
  noHealth:     { en: "No sensor node has reported yet.", bn: "এখনো কোনো সেন্সর নোড তথ্য পাঠায়নি।" },
  adminVisitorLink: { en: "Visitor view: open http://192.168.4.1/ on a phone",
                  bn: "দর্শনার্থীর পাতা: ফোনে http://192.168.4.1/ খুলুন" },
  footerLine:   { en: "WRO 2026 · Robots Meet Culture · Elementary",
                  bn: "WRO ২০২৬ · রোবট ও সংস্কৃতির মেলবন্ধন · এলিমেন্টারি" },
  statusPending:  { en: "pending",  bn: "অপেক্ষমাণ" },
  statusApproved: { en: "approved", bn: "অনুমোদিত" },
  statusDenied:   { en: "denied",   bn: "প্রত্যাখ্যাত" },
  statusUnknown:  { en: "unknown",  bn: "অজানা" }
```

---

## B. `webapp/admin.html` — remove judge-facing developer text

### B1. Untranslated test button

FIND
```
        <button class="secondary" onclick="Hub.advanceRobot(); refresh()">▶ Advance robot (test)</button>
```
REPLACE
```
        <button class="secondary" data-i18n="advanceRobot" onclick="Hub.advanceRobot(); refresh()">▶ Advance robot (test)</button>
```

### B2. Stale "Phase 6 / mock values" note under the health table

FIND
```
      <p class="muted" style="margin-top:8px;">Temp/Hum/Gas are mock values now. In Phase 6 they arrive live from each artifact's DHT11 + MQ-135 over ESP-NOW.</p>
```
REPLACE
```
      <p class="muted" style="margin-top:8px;" data-i18n="healthNote">Readings arrive live from each artifact's DHT11 + MQ-135 node over ESP-NOW.</p>
```
The old line is wrong on the real Hub (values are not mocked there — the table is
simply empty until a health node reports) and "Phase 6" means nothing to a judge.

### B3. Developer footer

FIND
```
    <p class="footer">Open <a href="index.html">index.html</a> in another tab to play a visitor.</p>
```
REPLACE
```
    <p class="footer" data-i18n="adminVisitorLink">Visitor view: open http://192.168.4.1/ on a phone</p>
```

### B4. Health table empty state

On the real Hub `state.health` is `[]` until a sensor node exists, so the table
currently renders as a heading over nothing. In `renderHealth()`:

FIND
```
    function renderHealth(health) {
      const lang = getLang();
      document.getElementById("healthBody").innerHTML = health.map((h) => {
```
REPLACE
```
    function renderHealth(health) {
      const lang = getLang();
      if (!health.length) {
        document.getElementById("healthBody").innerHTML =
          `<tr><td colspan="5" class="muted">${t("noHealth")}</td></tr>`;
        return;
      }
      document.getElementById("healthBody").innerHTML = health.map((h) => {
```

---

## C. `webapp/visitor.html`

### C1. "Now explaining" duplicates the station name

The robot card shows `🧵 Jamdani Saree` and directly under it `Now explaining: Jamdani
Saree`. Show the artifact's description instead — it's the line a visitor actually reads.

FIND
```
      document.getElementById("robotNarration").textContent = cur.name[lang];
```
REPLACE
```
      document.getElementById("robotNarration").textContent = cur.desc[lang];
```

### C2. Check-in status shown to the visitor in raw English

FIND
```
        Hub.getRequest(id).then((r) => { if (r) note.textContent = id + " · " + r.status; });
```
REPLACE
```
        const STATUS_KEY = { pending: "statusPending", approved: "statusApproved",
                             denied: "statusDenied", unknown: "statusUnknown" };
        Hub.getRequest(id).then((r) => {
          if (r) note.textContent = id + " · " + t(STATUS_KEY[r.status] || "statusUnknown");
        });
```

### C3. Footer line (also `index.html`)

FIND (both files)
```
    <p class="footer">WRO 2026 · Robots Meet Culture · Elementary</p>
```
REPLACE
```
    <p class="footer" data-i18n="footerLine">WRO 2026 · Robots Meet Culture · Elementary</p>
```

---

## D. `webapp/css/style.css` — typography + accessibility polish

Append at the end of the file. Nothing above is modified, so this is the block least
likely to collide with the gallery session's CSS.

```css
/* ---------- Bangla typography ----------
   uppercase does nothing to Bangla, and letter-spacing pulls conjuncts apart,
   so drop both when the page is in Bangla. */
html[lang="bn"] .brand .tag,
html[lang="bn"] .stat .lbl,
html[lang="bn"] th {
  text-transform: none;
  letter-spacing: normal;
}

/* ---------- keyboard focus (the admin runs on a laptop) ---------- */
button:focus-visible,
.btn:focus-visible,
.lang-toggle:focus-visible {
  outline: 3px solid var(--terracotta-dark);
  outline-offset: 2px;
}

/* ---------- contrast: #c98a00 on #fff4e0 is under 4.5:1 ---------- */
.pill.watch { color: #8a5d00; }

/* ---------- respect reduced-motion preferences ---------- */
@media (prefers-reduced-motion: reduce) {
  .emergency-banner { animation: none; }
  .dot.moving { animation: none; }
  .spinner { animation-duration: 2s; }
}
```

---

## E. After applying

1. Syntax-check the JS:
   ```bash
   node --check webapp/js/data.js && node --check webapp/js/hub.js
   ```
2. **Re-sync the flashed copy** — `webapp/` and `firmware/hub/data/` have drifted
   (`admin.html`, `js/data.js`, `js/hub.js`, `css/style.css` differ as of 15:51):
   ```bash
   cd /home/user/Projects/WRO_Rizwanul-Rafeeul-Luban
   cp webapp/index.html webapp/visitor.html webapp/admin.html firmware/hub/data/
   cp webapp/css/style.css firmware/hub/data/css/
   cp webapp/js/data.js webapp/js/hub.js webapp/js/qr.js firmware/hub/data/js/
   for f in index.html admin.html visitor.html js/data.js js/hub.js js/qr.js css/style.css; do
     diff -q webapp/$f firmware/hub/data/$f || echo "DRIFT: $f"
   done
   ```
   Do **not** copy `webapp/js/vendor/jsQR.js` blindly — confirm it is still present in
   `firmware/hub/data/js/vendor/` (it is, 256 885 bytes).
3. Flash the filesystem, then hard-reload the phone/laptop browsers:
   ```bash
   cd firmware/hub && pio run -t uploadfs
   ```
4. Check both languages on both pages with the header toggle — especially the
   emergency banner and the health pills.
