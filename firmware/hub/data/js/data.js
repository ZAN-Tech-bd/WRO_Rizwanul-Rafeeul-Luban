// =====================================================================
//  data.js  —  artifact content + bilingual text (Bangla / English)
//  Part of the Smart Museum web app. Safe to edit — this is the content
//  the team owns: the exhibits and the words on screen.
// =====================================================================

// The 7 Bangladeshi cultural artifacts. Each is one "station" the robot
// can stop at, and one card on the visitor dashboard.
// display = the real object shown on the booth table next to the station.
const ARTIFACTS = [
  {
    id: "jamdani",
    emoji: "🏺",
    display: { en: "Piece of saree", bn: "শাড়ির কাপড়" },
    name: { en: "Terracotta Plaques", bn: "টেরাকোটা ফলক" },
    desc: {
      en: "A world-famous handwoven fabric from Bangladesh, known for its beautiful patterns.",
      bn: "বাংলাদেশের বিখ্যাত হাতে বোনা কাপড়, যা তার সুন্দর নকশার জন্য পরিচিত।"
    }
  },
  {
    id: "nakshi",
    emoji: "🛶",
    display: { en: "Piece of nakshi kantha", bn: "নকশি কাঁথার টুকরো" },
    name: { en: "Traditional Wooden Boat", bn: "ঐতিহ্যবাহী কাঠের নৌকা" },
    desc: {
      en: "Traditional embroidered quilts made by hand using colourful stitching.",
      bn: "রঙিন সেলাই দিয়ে হাতে তৈরি ঐতিহ্যবাহী কাঁথা।"
    }
  },
  {
    id: "liberation",
    emoji: "🪡",
    display: { en: "3D print", bn: "থ্রিডি প্রিন্ট" },
    name: { en: "Nakshi Kantha", bn: "নকশি কাঁথা" },
    desc: {
      en: "Items such as uniforms, medals, maps and historical photographs from the 1971 Liberation War.",
      bn: "১৯৭১ সালের মুক্তিযুদ্ধের ইউনিফর্ম, পদক, মানচিত্র ও ঐতিতাসিক ছবির মতো নিদর্শন।"
    }
  },
  {
    id: "terracotta",
    emoji: "🪔",
    display: { en: "Bought at store", bn: "দোকান থেকে কেনা" },
    name: { en: "Traditional Bangladeshi Pottery", bn: "ঐতিহ্যবাহী বাংলাদেশি মৃৎশিল্প" },
    desc: {
      en: "Decorative baked-clay artworks found in ancient temples and archaeological sites.",
      bn: "প্রাচীন মন্দির ও প্রত্নস্থলে পাওয়া পোড়ামাটির সাজানো শিল্পকর্ম।"
    }
  },
  {
    id: "ektara",
    emoji: "🪑",
    display: { en: "Ektara instrument", bn: "একতারা বাদ্যযন্ত্র" },
    name: { en: "Traditional Brass Chair", bn: "ঐতিহ্যবাহী পিতলের চেয়ার" },
    desc: {
      en: "A one-string musical instrument commonly used by Baul singers.",
      bn: "বাউল সঙ্গীতশিল্পীদের ব্যবহৃত একটানা বাদ্যযন্ত্র।"
    }
  },
  {
    id: "palki",
    emoji: "🎋",
    display: { en: "Palki model", bn: "পালকির মডেল" },
    name: { en: "Bamboo and Cane", bn: "বাঁশ ও বেত" },
    desc: {
      en: "A decorated carriage once used to carry important people before modern transport.",
      bn: "আধুনিক যানের আগে গুরুত্বপূর্ণ মানুষদের বহন করতে ব্যবহৃত সজ্জিত যান।"
    }
  },
  {
    id: "tiger",
    emoji: "🧵",
    display: { en: "Tiger exhibit", bn: "বাঘের প্রদর্শনী" },
    name: { en: "Jamdani Saree", bn: "জামদানি শাড়ি" },
    desc: {
      en: "An educational display about Bangladesh's national animal and the Sundarbans.",
      bn: "বাংলাদেশের জাতীয় পশু ও সুন্দরবন সম্পর্কে একটি তথ্যমূলক প্রদর্শনী।"
    }
  }
];

// UI text in both languages. t("key") returns the string for the current language.
const I18N = {
  appName:      { en: "Smart Museum",            bn: "স্মার্ট মিউজিয়াম" },
  tagline:      { en: "Robots Meet Culture",     bn: "রোবট মিটস কালচার" },
  gateTitle:    { en: "Visitor Check-in",        bn: "ভিজিটর চেক-ইন" },
  gateHint:     { en: "Scan the QR code at the gate to request entry.", bn: "প্রবেশের অনুরোধ করতে গেটের কিউআর কোড স্ক্যান করুন।" },
  gateScan:     { en: "Scan QR",                 bn: "কিউআর স্ক্যান" },
  gateSimulate: { en: "Simulate scan (demo)",    bn: "সিমুলেট স্ক্যান (ডেমো)" },
  gateReading:  { en: "Reading QR code…",        bn: "কিউআর কোড পড়া হচ্ছে…" },
  gateUnreadable:{ en: "QR code not found. Hold the camera steady and try again.", bn: "কিউআর কোড পাওয়া যায়নি। ক্যামেরা স্থির রেখে আবার চেষ্টা করুন।" },
  gateInvalid:  { en: "This is not the museum entry QR code.", bn: "এটি মিউজিয়ামের প্রবেশ কিউআর কোড নয়।" },
  gateScannerUnavailable:{ en: "QR scanner failed to load. Please use the native check-in QR.", bn: "কিউআর স্ক্যানার লোড হয়নি। নেটিভ চেক-ইন কিউআর ব্যবহার করুন।" },
  gateWaiting:  { en: "Waiting for admin approval…", bn: "অ্যাডমিনের অনুমোদনের জন্য অপেক্ষা করা হচ্ছে…" },
  gateApproved: { en: "Approved! Opening dashboard…", bn: "অনুমোদিত! ড্যাশবোর্ড খোলা হচ্ছে…" },
  gateDenied:   { en: "Not approved. Please see the desk.", bn: "অনুমোদিত হয়নি। ডেস্কে যোগাযোগ করুন।" },
  visitorBadge: { en: "Visitor",                 bn: "ভিজিটর" },
  adminBadge:   { en: "Admin",                   bn: "অ্যাডমিন" },
  robotNow:     { en: "The robot is at",         bn: "রোবট এখন আছে" },
  narrating:    { en: "Now explaining",          bn: "এখন বর্ণনা করছে" },
  nextStation:  { en: "Next stop",               bn: "পরবর্তী স্টেশন" },
  queueWait:    { en: "Please wait your turn and follow the queue.", bn: "অনুগ্রহ করে আপনার পালার জন্য অপেক্ষা করুন এবং লাইন অনুসরণ করুন।" },
  emergencyMsg: { en: "EMERGENCY — follow the robot to the nearest exit.", bn: "জরুরি অবস্থা — নিকটতম এক্সিটে যেতে রোবটের সাথে চলুন।" },
  artifactsTitle:{ en: "Tour Sequence",          bn: "ভ্রমণ ক্রম" },
  tourHint:      { en: "Lost the robot? Find it heading to the next stop in this order.", bn: "রোবট হারিয়ে গেছে? এই ক্রমানুসারে পরবর্তী স্টেশনে এগিয়ে যাওয়া রোবটটি খুঁজে নিন।" },
  pendingTitle: { en: "Pending approvals",       bn: "অপেক্ষমাণ অনুমোদন" },
  approve:      { en: "Approve",                 bn: "অনুমোদন" },
  deny:         { en: "Deny",                    bn: "প্রত্যাখ্যান" },
  visitorCount: { en: "Visitors inside",         bn: "ভেতরে ভিজিটর" },
  waitingCount: { en: "Waiting",                 bn: "অপেক্ষমাণ" },
  healthTitle:  { en: "Artifact health",         bn: "নিদর্শনের অবস্থা" },
  thArtifact:   { en: "Artifact",                bn: "নিদর্শন" },
  thTemp:       { en: "Temp °C",                 bn: "তাপ °C" },
  thHum:        { en: "Humidity %",              bn: "আর্দ্রতা %" },
  thGas:        { en: "Gas",                     bn: "গ্যাস" },
  thStatus:     { en: "Status",                  bn: "অবস্থা" },
  display:      { en: "Display",                 bn: "প্রদর্শন" },
  noPending:    { en: "No one is waiting.",      bn: "কেউ অপেক্ষা করছে না।" },
  emergencyOn:  { en: "End emergency",           bn: "জরুরি অবস্থা শেষ" },
  emergencyOff: { en: "Start emergency (test)",  bn: "জরুরি অবস্থা শুরু (পরীক্ষা)" },
  robotStatus:  { en: "Robot status",            bn: "রোবটের অবস্থা" },
  // Same convention as emergencyOn/Off: the key names the state the robot is
  // ALREADY in, and the label says what pressing the button will do next.
  muteOn:       { en: "🔊 Unmute robot",          bn: "🔊 রোবটের শব্দ চালু" },
  muteOff:      { en: "🔇 Mute robot",            bn: "🔇 রোবটের শব্দ বন্ধ" },
  mutedBadge:   { en: "Audio muted",             bn: "শব্দ বন্ধ" },
  good:         { en: "Good",                    bn: "ভালো" },
  watch:        { en: "Watch",                   bn: "সতর্ক" },
  alert:        { en: "Alert",                   bn: "সংকেত" },
  cameraTitle:  { en: "Camera",                  bn: "ক্যামেরা" },
  cameraWaiting:{ en: "Waiting for the ESP-CAM to send a photo — send SHOOT on the camera's serial console.", bn: "ESP-CAM থেকে ছবি আসার অপেক্ষা — ক্যামেরার সিরিয়ালে SHOOT পাঠান।" },
  cameraEarlier:{ en: "Earlier shots",           bn: "আগের ছবি" },
  cameraEarlierBoot:{ en: "before this restart", bn: "এই রিস্টার্টের আগে" },
  cameraAgo:    { en: "ago",                     bn: "আগে" },
  cameraStored: { en: "stored",                  bn: "সংরক্ষিত" },
  cameraClear:  { en: "Clear photos",            bn: "ছবি মুছুন" },
  cameraClearAsk:{ en: "Delete every stored photo?", bn: "সংরক্ষিত সব ছবি মুছে ফেলবেন?" },
  sensorTitle:  { en: "Hub sensors — pin scan",  bn: "হাব সেন্সর — পিন স্ক্যান" },
  sensorHint:   { en: "Each GPIO wired to the Hub is probed as a DHT11 in turn. \"Answering\" means a real DHT11 replied on that pin. Pins marked input-only (GPIO34–39) have no output driver, so a DHT11 can never start there — read their analog column instead, which is what an MQ sensor needs.",
                  bn: "হাবে লাগানো প্রতিটি GPIO পালাক্রমে DHT11 হিসেবে পরীক্ষা করা হয়। \"সাড়া দিচ্ছে\" মানে ওই পিনে সত্যিই DHT11 আছে। input-only চিহ্নিত পিনে (GPIO34–39) আউটপুট ড্রাইভার নেই, তাই সেখানে DHT11 কাজ করবে না — সেগুলোর অ্যানালগ কলাম দেখুন, MQ সেন্সরের জন্য সেটাই দরকার।" },
  sensorNone:   { en: "No DHT11 has answered yet — a full sweep takes about 20 s.",
                  bn: "এখনো কোনো DHT11 সাড়া দেয়নি — একটি পূর্ণ স্ক্যানে প্রায় ২০ সেকেন্ড লাগে।" },
  sensorFound:  { en: "DHT11 answering on",      bn: "DHT11 সাড়া দিচ্ছে" },
  thPin:        { en: "GPIO",                    bn: "GPIO" },
  thAnalog:     { en: "Analog (MQ)",             bn: "অ্যানালগ (MQ)" },
  thReads:      { en: "Reads",                   bn: "রিড" },
  thResult:     { en: "Result",                  bn: "ফলাফল" },
  dhtOk:        { en: "answering",               bn: "সাড়া দিচ্ছে" },
  dhtWait:      { en: "not scanned yet",         bn: "এখনো স্ক্যান হয়নি" },
  dhtLow:       { en: "pin held LOW",            bn: "পিন LOW আটকে আছে" },
  dhtNone:      { en: "no DHT11 answer",         bn: "DHT11 সাড়া দেয়নি" },
  dhtCut:       { en: "frame cut short",         bn: "ফ্রেম অসম্পূর্ণ" },
  dhtSum:       { en: "bad checksum",            bn: "চেকসাম ভুল" },
  inputOnly:    { en: "input-only",              bn: "input-only" },
  noAdc:        { en: "no ADC",                  bn: "ADC নেই" },
  healthEmpty:  { en: "No readings yet.",        bn: "এখনো কোনো রিডিং নেই।" },
  gasGood:      { en: "Good air",                bn: "বাতাস ভালো" },
  gasBad:       { en: "Gas detected",            bn: "গ্যাস শনাক্ত" },
  healthNote:   { en: "Temp/humidity come from the Hub's DHT11 pins, gas from its MQ pins. Pin-to-artifact mapping lives in HUB_SENSOR_STATION in include/config.h.",
                  bn: "তাপ/আর্দ্রতা আসে হাবের DHT11 পিন থেকে, গ্যাস আসে MQ পিন থেকে। পিন-টু-নিদর্শন ম্যাপিং আছে include/config.h ফাইলের HUB_SENSOR_STATION-এ।" }
};

// ---- language helpers -------------------------------------------------
const LANG_KEY = "museum:lang";

function getLang() {
  return localStorage.getItem(LANG_KEY) || "bn"; // default Bangla
}

function setLang(lang) {
  localStorage.setItem(LANG_KEY, lang);
  applyLang();
}

// text for a key in the current language
function t(key) {
  const lang = getLang();
  const entry = I18N[key];
  if (!entry) return key;
  return entry[lang] || entry.en;
}

// refresh every element marked data-i18n="key" on the page
function applyLang() {
  const lang = getLang();
  document.documentElement.lang = lang === "bn" ? "bn" : "en";
  document.querySelectorAll("[data-i18n]").forEach((el) => {
    const key = el.getAttribute("data-i18n");
    if (I18N[key]) el.textContent = I18N[key][lang] || I18N[key].en;
  });
  document.querySelectorAll("[data-i18n-ph]").forEach((el) => {
    const key = el.getAttribute("data-i18n-ph");
    if (I18N[key]) el.placeholder = I18N[key][lang] || I18N[key].en;
  });
  // tell the page it can re-render language-dependent lists now
  document.dispatchEvent(new Event("langchange"));
}

// small helper to build the language toggle button used on every page
function mountLangToggle(containerId) {
  const c = document.getElementById(containerId);
  if (!c) return;
  c.innerHTML = "";
  const btn = document.createElement("button");
  btn.className = "lang-toggle";
  const refresh = () => {
    btn.textContent = getLang() === "bn" ? "EN" : "বাংলা";
  };
  refresh();
  btn.onclick = () => {
    setLang(getLang() === "bn" ? "en" : "bn");
    refresh();
  };
  c.appendChild(btn);
}
