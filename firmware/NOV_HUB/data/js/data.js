/* NOV_HUB offline catalog. Order is the robot's numeric station order. */
const ARTIFACTS = [
  { id: 'terracotta', station: 1, uid: '64 14 F6 B3', simulated: false,
    name: { en: 'Terracotta Plaque', bn: 'টেরাকোটা ফলক' },
    display: { en: 'Decorated fired-clay plaque', bn: 'নকশা করা পোড়ামাটির ফলক' },
    desc: { en: 'Stories pressed into clay. Floral and geometric reliefs connect this plaque to the terracotta decoration of Bengal’s historic buildings.', bn: 'মাটির গায়ে আঁকা গল্প। ফুল ও জ্যামিতিক নকশার এই ফলক বাংলার ঐতিহাসিক স্থাপনার পোড়ামাটির অলংকরণের কথা মনে করিয়ে দেয়।' } },
  { id: 'woodenboat', station: 2, uid: '4A 6B DF 34', simulated: false,
    name: { en: 'Wooden boat', bn: 'কাঠের নৌকা' },
    display: { en: 'Traditional wooden boat', bn: 'ঐতিহ্যবাহী কাঠের নৌকা' },
    desc: { en: 'Made for a river country. The curved hull recalls the boats that carry people, goods and everyday life along Bangladesh’s waterways.', bn: 'নদীমাতৃক দেশের বাহন। বাঁকানো এই নৌকা বাংলাদেশের নদীপথে মানুষ, পণ্য ও দৈনন্দিন জীবন বহনকারী নৌকার কথা বলে।' } },
  { id: 'rickshaw', station: 3, uid: 'D8 36 D4 B9', simulated: false,
    name: { en: 'Rickshaw Art', bn: 'রিকশাচিত্র' },
    display: { en: 'Painted rickshaw artwork', bn: 'রিকশার আঁকা শিল্পকর্ম' },
    desc: { en: 'A moving canvas on city streets. Bright flowers, birds and scenes of rural life turn the rickshaw into a familiar expression of Bangladeshi folk art.', bn: 'শহরের পথে চলমান ক্যানভাস। উজ্জ্বল ফুল, পাখি ও গ্রামীণ জীবনের দৃশ্য রিকশাকে বাংলাদেশের লোকশিল্পের পরিচিত রূপ দিয়েছে।' } },
  { id: 'pottery', station: 4, uid: '94 60 6B B2', simulated: false,
    name: { en: 'Traditional pottery', bn: 'ঐতিহ্যবাহী মৃৎশিল্প' },
    display: { en: 'Hand-shaped clay vessel', bn: 'হাতে গড়া মাটির পাত্র' },
    desc: { en: 'Earth, water and a potter’s hands. Clay vessels have long been part of cooking, storing water and daily rituals in Bengal.', bn: 'মাটি, পানি ও কুমারের হাত। রান্না, পানি রাখা ও বাংলার দৈনন্দিন আচার-অনুষ্ঠানে মাটির পাত্রের ব্যবহার বহু পুরোনো।' } },
  { id: 'flute', station: 5, uid: '18 48 F4 BA', simulated: false,
    name: { en: 'Bamboo flute', bn: 'বাঁশের বাঁশি' },
    display: { en: 'Bamboo wind instrument', bn: 'বাঁশের তৈরি সুষির বাদ্যযন্ত্র' },
    desc: { en: 'A length of bamboo becomes a voice. Breath and finger holes shape the gentle sound heard in folk tunes across Bangladesh.', bn: 'এক টুকরো বাঁশ হয়ে ওঠে সুরের ভাষা। শ্বাস ও আঙুলের ছোঁয়ায় তৈরি কোমল সুর বাংলাদেশের লোকসংগীতের পরিচিত অংশ।' } },
  { id: 'basket', station: 6, uid: '94 58 1A B3', simulated: false,
    name: { en: 'Bamboo basket', bn: 'বাঁশের ঝুড়ি' },
    display: { en: 'Woven bamboo basket', bn: 'বোনা বাঁশের ঝুড়ি' },
    desc: { en: 'Useful things, carefully woven. Split bamboo is interlaced into a light, strong basket, showing a craft passed between generations.', bn: 'যত্নে বোনা নিত্যপ্রয়োজনীয় জিনিস। চেরা বাঁশ বুনে তৈরি হালকা ও মজবুত ঝুড়ি প্রজন্ম থেকে প্রজন্মে চলা কারুশিল্পের পরিচয় দেয়।' } }
];
const ROUTE_METADATA = {
  start: { uid: '83 54 6A 28', name: 'Start' }, end: { uid: '13 8C 5A 28', name: 'End' },
  reserved: [{ id: 'jamdani', name: 'Jamdani shari', uid: '09 4C B1 92', active: false }],
  note: 'RFID belongs to the robot. Hub receives numeric station IDs; these UIDs are reference metadata only.'
};
// Every visible interface phrase has an offline Bengali equivalent.
const I18N = {};
const TEXT = {
  appName: ['Museum of everyday Bengal', 'বাংলার জীবন ও ঐতিহ্য'],
  tagline: ['Robots meet culture · Bangladesh', 'রোবট ও সংস্কৃতি · বাংলাদেশ'],
  entry: ['Visitor entry', 'দর্শনার্থীর প্রবেশ'], desk: ['Control desk', 'নিয়ন্ত্রণ ডেস্ক'], visit: ['Gallery guide', 'গ্যালারি গাইড'],
  welcomeEyebrow: ['A guided journey · Six stops', 'রোবটের সাথে ভ্রমণ · ছয়টি স্টেশন'],
  welcomeTitle: ['A river of stories.\nA route through Bengal.', 'গল্পের নদী বেয়ে\nবাংলার ঐতিহ্যে।'],
  welcomeText: ['Clay, colour, bamboo and the river. Follow our museum robot through six objects from the everyday life of Bangladesh.', 'মাটি, রং, বাঁশ আর নদী। বাংলাদেশের দৈনন্দিন জীবনের ছয়টি নিদর্শন দেখতে আমাদের মিউজিয়াম রোবটকে অনুসরণ করুন।'],
  routeTitle: ['Your route through the collection', 'নিদর্শন দেখার পথ'],
  routeHint: ['Follow the numbered stops in order. The robot reports its position along this route.', 'সংখ্যার ক্রমে স্টেশনগুলো দেখুন। এই পথে রোবট তার অবস্থান জানায়।'],
  liveLegend: ['Collection route', 'নিদর্শনের ভ্রমণপথ'], demoLegend: ['6 stations', '৬টি স্টেশন'],
  real: ['Real station', 'বাস্তব স্টেশন'], simulated: ['Reading', 'রিডিং'], measured: ['Measured', 'পরিমাপ করা'], unavailable: ['Unavailable', 'পাওয়া যায়নি'],
  gateTitle: ['Begin your visit', 'আপনার ভ্রমণ শুরু করুন'],
  gateHint: ['Photograph the entry QR code, then wait for approval at the desk.', 'প্রবেশের কিউআর কোডের ছবি তুলুন, তারপর ডেস্কের অনুমোদনের জন্য অপেক্ষা করুন।'],
  gateScan: ['Scan entry QR', 'প্রবেশের কিউআর স্ক্যান করুন'], gateSimulate: ['Request entry', 'প্রবেশের অনুরোধ করুন'],
  jumpEntry: ['Go to check-in ↓', 'চেক-ইনে যান ↓'],
  stepScan: ['Scan the entry QR', 'প্রবেশের কিউআর স্ক্যান'], stepApprove: ['Wait for desk approval', 'ডেস্কের অনুমোদনের অপেক্ষা'], stepFollow: ['Follow your robot guide', 'রোবট গাইডকে অনুসরণ'],
  gateReading: ['Reading QR code…', 'কিউআর কোড পড়া হচ্ছে…'],
  gateUnreadable: ['No QR code found. Take a clear photo and try again.', 'কিউআর কোড পাওয়া যায়নি। পরিষ্কার ছবি তুলে আবার চেষ্টা করুন।'],
  gateInvalid: ['This is not the museum entry QR code.', 'এটি মিউজিয়ামের প্রবেশ কিউআর কোড নয়।'],
  gateScannerUnavailable: ['The local QR decoder is unavailable. Scan the entry QR with your phone’s camera instead.', 'স্থানীয় কিউআর ডিকোডার পাওয়া যায়নি। ফোনের ক্যামেরা দিয়ে প্রবেশের কিউআর স্ক্যান করুন।'],
  gateWaiting: ['Entry requested. Waiting for desk approval…', 'প্রবেশের অনুরোধ করা হয়েছে। ডেস্কের অনুমোদনের অপেক্ষা…'],
  gateApproved: ['Entry approved. Opening your gallery guide…', 'প্রবেশ অনুমোদিত। গ্যালারি গাইড খোলা হচ্ছে…'],
  gateDenied: ['Entry not approved. Please speak to the museum desk. You can try again.', 'প্রবেশ অনুমোদিত হয়নি। মিউজিয়াম ডেস্কে যোগাযোগ করুন। আবার চেষ্টা করতে পারেন।'],
  requestError: ['Could not complete the request. Check the Hub connection and try again.', 'অনুরোধ সম্পন্ন হয়নি। হাবের সংযোগ পরীক্ষা করে আবার চেষ্টা করুন।'],
  requestUnknown: ['This entry request is no longer available. Please scan again.', 'এই প্রবেশের অনুরোধ আর পাওয়া যাচ্ছে না। আবার স্ক্যান করুন।'],
  preview: ['Local view', 'স্থানীয় ভিউ'],
  previewHint: ['Open the control desk to manage entry requests.', 'প্রবেশের অনুরোধ পরিচালনা করতে নিয়ন্ত্রণ ডেস্ক খুলুন।'],
  connected: ['Hub connected · offline network', 'হাব সংযুক্ত · স্থানীয় নেটওয়ার্ক'], connecting: ['Connecting to museum Hub…', 'মিউজিয়াম হাবে সংযোগ হচ্ছে…'],
  disconnected: ['Hub connection lost — readings are not live. Reconnect to the Hub Wi-Fi.', 'হাবের সংযোগ বিচ্ছিন্ন — রিডিং লাইভ নয়। হাবের ওয়াই-ফাইয়ে আবার যুক্ত হোন।'],
  lastUpdate: ['Last response', 'সর্বশেষ সাড়া'], retry: ['Retry connection', 'আবার সংযোগ করুন'], never: ['No response yet', 'এখনো সাড়া নেই'],
  visitorTitle: ['Let the objects lead the way.', 'নিদর্শনের পথ ধরে চলুন।'], visitorIntro: ['Stay with the robot and follow the stops below.', 'রোবটের সাথে থাকুন এবং নিচের স্টেশনগুলো অনুসরণ করুন।'],
  robotNow: ['Robot guide', 'রোবট গাইড'], narrating: ['Narrating at', 'বর্ণনা করছে'], moving: ['Moving toward', 'এগিয়ে যাচ্ছে'],
  positionUnknown: ['Position not reported', 'অবস্থান জানানো হয়নি'], nextStation: ['Next stop', 'পরবর্তী স্টেশন'], routeEnd: ['End of the route', 'ভ্রমণপথের শেষ'],
  waitingRobot: ['Waiting for a robot position report. No stop is assumed.', 'রোবটের অবস্থানের অপেক্ষা। কোনো স্টেশন ধরে নেওয়া হয়নি।'],
  here: ['Robot here', 'রোবট এখানে'], robotMuted: ['Robot narration is muted.', 'রোবটের বর্ণনার শব্দ বন্ধ।'], display: ['On display', 'প্রদর্শনীতে'],
  emergencyMsg: ['EMERGENCY — follow the robot to the nearest exit. Follow staff instructions.', 'জরুরি অবস্থা — নিকটতম বের হওয়ার পথে রোবটকে অনুসরণ করুন। কর্মীদের নির্দেশ মানুন।'],
  deskEyebrow: ['NOV_HUB · Museum operations', 'NOV_HUB · মিউজিয়াম পরিচালনা'], deskTitle: ['Keep the visit flowing.', 'ভ্রমণ চলুক স্বচ্ছন্দে।'],
  deskIntro: ['Approve visitors, guide the robot and inspect the collection’s sensor readings.', 'দর্শনার্থীকে অনুমোদন দিন, রোবট চালান ও নিদর্শনের সেন্সর রিডিং দেখুন।'],
  visitorCount: ['Approved visitors', 'অনুমোদিত দর্শনার্থী'], waitingCount: ['Waiting for entry', 'প্রবেশের অপেক্ষায়'], pendingTitle: ['Entry requests', 'প্রবেশের অনুরোধ'],
  noPending: ['No visitors waiting', 'কোনো দর্শনার্থী অপেক্ষায় নেই'], noPendingHint: ['New entry requests will appear here after a QR scan.', 'কিউআর স্ক্যানের পর নতুন প্রবেশের অনুরোধ এখানে দেখা যাবে।'],
  approve: ['Approve entry', 'প্রবেশ অনুমোদন'], deny: ['Deny entry', 'প্রবেশ প্রত্যাখ্যান'], requestId: ['Request', 'অনুরোধ'],
  robotStatus: ['Robot & safety', 'রোবট ও নিরাপত্তা'], advance: ['Advance robot (test)', 'রোবট এগিয়ে নিন (পরীক্ষা)'],
  muteOn: ['Unmute robot', 'রোবটের শব্দ চালু করুন'], muteOff: ['Mute robot', 'রোবটের শব্দ বন্ধ করুন'], mutedBadge: ['Audio muted', 'শব্দ বন্ধ'],
  emergencyOn: ['End emergency', 'জরুরি অবস্থা শেষ করুন'], emergencyOff: ['Start emergency (test)', 'জরুরি অবস্থা শুরু করুন (পরীক্ষা)'],
  safetyNote: ['Emergency mode directs visitors to the exit. Mute controls narration only.', 'জরুরি মোড দর্শনার্থীকে বের হওয়ার পথে নির্দেশ করে। মিউট শুধু বর্ণনার শব্দ নিয়ন্ত্রণ করে।'],
  commandSent: ['Command sent. The next Hub response confirms the state.', 'নির্দেশ পাঠানো হয়েছে। হাবের পরবর্তী সাড়া অবস্থা নিশ্চিত করবে।'],
  healthTitle: ['Collection environment', 'নিদর্শনের পরিবেশ'], healthSub: ['Station readings and gas status.', 'স্টেশনের রিডিং ও গ্যাসের অবস্থা।'],
  healthNote: ['Gas status reflects changes from each sensor’s baseline.', 'গ্যাসের অবস্থা প্রতিটি সেন্সরের স্বাভাবিক রিডিংয়ের পরিবর্তন দেখায়।'],
  thArtifact: ['Stop / artifact', 'স্টেশন / নিদর্শন'], thTemp: ['Temperature', 'তাপমাত্রা'], thHum: ['Humidity', 'আর্দ্রতা'], thGas: ['Gas status', 'গ্যাসের অবস্থা'], thProx: ['Proximity', 'নিকটবর্তী উপস্থিতি'],
  gasNormal: ['Normal', 'স্বাভাবিক'], gasElevated: ['Elevated', 'বাড়তি'], gasHigh: ['High', 'উচ্চ'], gasLearning: ['Starting up', 'চালু হচ্ছে'],
  proxStations: ['Stations', 'স্টেশন'], proxDetected: ['Detected', 'উপস্থিতি শনাক্ত'],
  proxRecent: ['Recently detected', 'সম্প্রতি উপস্থিতি শনাক্ত'], proxClear: ['Clear', 'উপস্থিতি শনাক্ত হয়নি'],
  sensorTitle: ['Sensor inputs', 'সেন্সর ইনপুট'], sensorHint: ['Station input details.', 'স্টেশন ইনপুটের বিস্তারিত।'],
  thPin: ['GPIO / role', 'GPIO / ভূমিকা'], thStop: ['Stop', 'স্টেশন'], thReading: ['Reading', 'রিডিং'], thReads: ['Valid / tries', 'সফল / চেষ্টা'], thResult: ['Source / diagnostic', 'উৎস / পরীক্ষা'],
  noSensors: ['Waiting for sensor diagnostics.', 'সেন্সর পরীক্ষার তথ্যের অপেক্ষা।'],
  cameraTitle: ['Camera journal', 'ক্যামেরার ছবি'], cameraSub: ['Photos from the robot’s station stops.', 'স্টেশনে রোবট থামলে তোলা ছবি।'],
  demoPhoto: ['Photo', 'ছবি'],
  cameraWaiting: ['No photographs received yet.', 'এখনো কোনো ছবি পাওয়া যায়নি।'], cameraPreview: ['No photographs available yet.', 'এখনো কোনো ছবি পাওয়া যায়নি।'],
  cameraEarlier: ['Earlier photographs', 'আগের ছবি'], cameraEarlierBoot: ['Before this restart', 'এই রিস্টার্টের আগে'], cameraAgo: ['ago', 'আগে'],
  cameraStored: ['photos', 'ছবি'], cameraClear: ['Clear Hub gallery', 'হাবের গ্যালারি খালি করুন'], cameraClearAsk: ['Delete all photos stored in the Hub gallery?', 'হাবের গ্যালারিতে রাখা সব ছবি মুছে ফেলবেন?'],
  cameraError: ['Could not load the gallery. Retry when the Hub is connected.', 'ছবি লোড হয়নি। হাব যুক্ত হলে আবার চেষ্টা করুন।'], imageError: ['Photo could not be loaded. Select it to retry.', 'ছবি লোড হয়নি। আবার চেষ্টা করতে ছবিটি বাছুন।'],
  close: ['Close viewer', 'ছবির ভিউয়ার বন্ধ'], older: ['Older photograph', 'আগের ছবি'], newer: ['Newer photograph', 'পরের ছবি'],
  visitorRequest: ['Visitor entry', 'দর্শনার্থীর প্রবেশ'], approved: ['Approved', 'অনুমোদিত'], pending: ['Pending', 'অপেক্ষমাণ'], denied: ['Denied', 'প্রত্যাখ্যাত'], unknown: ['Unknown', 'অজানা'],
  footer: ['WRO 2026 · Future Innovators · South Breeze School, Bangladesh', 'WRO ২০২৬ · ফিউচার ইনোভেটরস · সাউথ ব্রিজ স্কুল, বাংলাদেশ'],
  credit: ['Built by Syed Rizwanul Alam · Syed Rafeeul Alam · Tareef Toufique', 'নির্মাতা: সৈয়দ রিজওয়ানুল আলম · সৈয়দ রাফিউল আলম · তারিফ তৌফিক'],
  offline: ['Local collection. No internet required.', 'স্থানীয় সংগ্রহ। ইন্টারনেটের প্রয়োজন নেই।'], skip: ['Skip to content', 'মূল অংশে যান']
};
Object.entries(TEXT).forEach(([key, value]) => { I18N[key] = { en: value[0], bn: value[1] }; });
const LANG_KEY = 'novhub:lang';
function getLang() { try { return localStorage.getItem(LANG_KEY) === 'en' ? 'en' : 'bn'; } catch (_) { return 'bn'; } }
function t(key) { return I18N[key]?.[getLang()] || key; }
function setLang(lang) { try { localStorage.setItem(LANG_KEY, lang); } catch (_) {} applyLang(); }
function applyLang() {
  document.documentElement.lang = getLang();
  document.querySelectorAll('[data-i18n]').forEach(el => { el.textContent = t(el.dataset.i18n); });
  document.querySelectorAll('[data-i18n-label]').forEach(el => { el.setAttribute('aria-label', t(el.dataset.i18nLabel)); });
  document.querySelectorAll('.lang-toggle').forEach(el => { el.textContent = getLang() === 'bn' ? 'English' : 'বাংলা'; el.setAttribute('aria-label', getLang() === 'bn' ? 'Switch to English' : 'বাংলায় দেখুন'); });
  document.dispatchEvent(new Event('langchange'));
}
function mountLangToggle(id) {
  const button = document.createElement('button'); button.type = 'button'; button.className = 'lang-toggle';
  button.addEventListener('click', () => setLang(getLang() === 'bn' ? 'en' : 'bn'));
  document.getElementById(id)?.replaceChildren(button);
}
function artifactById(id) { return ARTIFACTS.find(a => a.id === id) || null; }
