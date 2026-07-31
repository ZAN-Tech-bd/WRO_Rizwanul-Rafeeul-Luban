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
    emoji: "🧵",
    display: { en: "Piece of saree", bn: "শাড়ির কাপড়" },
    name: { en: "Jamdani Saree", bn: "জামদানি শাড়ি" },
    desc: {
      en: "A world-famous handwoven fabric from Bangladesh, known for its beautiful patterns.",
      bn: "বাংলাদেশের বিখ্যাত হাতে বোনা কাপড়, যা তার সুন্দর নকশার জন্য পরিচিত।"
    }
  },
  {
    id: "nakshi",
    emoji: "🪡",
    display: { en: "Piece of nakshi kantha", bn: "নকশি কাঁথার টুকরো" },
    name: { en: "Nakshi Kantha", bn: "নকশি কাঁথা" },
    desc: {
      en: "Traditional embroidered quilts made by hand using colourful stitching.",
      bn: "রঙিন সেলাই দিয়ে হাতে তৈরি ঐতিহ্যবাহী কাঁথা।"
    }
  },
  {
    id: "liberation",
    emoji: "🇧🇩",
    display: { en: "3D print", bn: "থ্রিডি প্রিন্ট" },
    name: { en: "Liberation War Artifacts", bn: "মুক্তিযুদ্ধের নিদর্শন" },
    desc: {
      en: "Items such as uniforms, medals, maps and historical photographs from the 1971 Liberation War.",
      bn: "১৯৭১ সালের মুক্তিযুদ্ধের ইউনিফর্ম, পদক, মানচিত্র ও ঐতিতাসিক ছবির মতো নিদর্শন।"
    }
  },
  {
    id: "terracotta",
    emoji: "🏺",
    display: { en: "Bought at store", bn: "দোকান থেকে কেনা" },
    name: { en: "Terracotta Plaques", bn: "টেরাকোটা ফলক" },
    desc: {
      en: "Decorative baked-clay artworks found in ancient temples and archaeological sites.",
      bn: "প্রাচীন মন্দির ও প্রত্নস্থলে পাওয়া পোড়ামাটির সাজানো শিল্পকর্ম।"
    }
  },
  {
    id: "ektara",
    emoji: "🎵",
    display: { en: "Ektara instrument", bn: "একতারা বাদ্যযন্ত্র" },
    name: { en: "Ektara", bn: "একতারা" },
    desc: {
      en: "A one-string musical instrument commonly used by Baul singers.",
      bn: "বাউল সঙ্গীতশিল্পীদের ব্যবহৃত একটানা বাদ্যযন্ত্র।"
    }
  },
  {
    id: "palki",
    emoji: "👑",
    display: { en: "Palki model", bn: "পালকির মডেল" },
    name: { en: "Palki (Palanquin)", bn: "পালকি" },
    desc: {
      en: "A decorated carriage once used to carry important people before modern transport.",
      bn: "আধুনিক যানের আগে গুরুত্বপূর্ণ মানুষদের বহন করতে ব্যবহৃত সজ্জিত যান।"
    }
  },
  {
    id: "tiger",
    emoji: "🐅",
    display: { en: "Tiger exhibit", bn: "বাঘের প্রদর্শনী" },
    name: { en: "Royal Bengal Tiger", bn: "রয়েল বেঙ্গল টাইগার" },
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
  artifactsTitle:{ en: "Our Artifacts",          bn: "আমাদের নিদর্শন" },
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
  good:         { en: "Good",                    bn: "ভালো" },
  watch:        { en: "Watch",                   bn: "সতর্ক" },
  alert:        { en: "Alert",                   bn: "সংকেত" }
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
