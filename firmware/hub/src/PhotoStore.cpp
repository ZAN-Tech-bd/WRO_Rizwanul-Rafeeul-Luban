// =====================================================================
//  PhotoStore.cpp  —  see include/PhotoStore.h for the contract.
// =====================================================================

#include "PhotoStore.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

#include "config.h"

namespace {

// Photo names are deliberately strict 8.3: "I00012S3.JPG" = 'I', a 5-digit
// sequence, 'S', a 1-digit station, ".JPG". Long names would push FATFS onto its
// long-file-name path, whose working buffers live on the caller's stack -- an
// expensive thing to do inside the web task. 8.3 keeps every card access cheap.
const char  kNamePrefix     = 'I';
const char  kStationMark    = 'S';
const char  kNameExt[]      = ".JPG";
const char  kSeqName[]      = "/SEQ.TXT";
const uint8_t kSeqDigits    = 5;      // I00012 -> zero-padded to 5
const uint16_t kSeqCeiling  = 65000;  // uint16_t headroom before a forced wipe

// The upload scratch file. Always LittleFS: see the comment on openUploadTemp()
// in PhotoStore.h -- SPI writes inside the HTTP parse loop truncate uploads.
const char  kTempPath[]     = "/upload.tmp";
const size_t kCopyChunk     = 512;    // LittleFS -> card copy buffer (stack)

// VFS mount points the Arduino wrappers use. Needed by exists() below.
const char kLittleFsMount[] = "/littlefs";
const char kSdMount[]       = "/sd";

// Strip any directory part so both "/photos/I00001S2.JPG" and "I00001S2.JPG"
// parse the same way (openNextFile() differs across core versions).
const char* baseName(const char* path) {
  const char* slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

// Arduino's FS::exists() probes by opening the file, so a miss prints
// "does not exist, no permits for creation" at ERROR level -- alarming log noise
// for a perfectly normal "no scratch file yet". stat() answers silently.
bool quietExists(const char* mount, const char* path) {
  char full[80];
  snprintf(full, sizeof(full), "%s%s", mount, path);
  struct stat st;
  return stat(full, &st) == 0;
}

}  // namespace

// ---------------------------------------------------------------------
//  Mounting
// ---------------------------------------------------------------------
// SPI cards on jumper wires are marginal: a card that mounts on one boot can
// answer "Select Failed" on the next. Retry a few times before giving up,
// because falling back to LittleFS silently loses the archive the user wanted.
bool PhotoStore::mountSd() {
  SPI.begin(HUB_SD_SCK_PIN, HUB_SD_MISO_PIN, HUB_SD_MOSI_PIN, HUB_SD_CS_PIN);

  for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
    if (SD.begin(HUB_SD_CS_PIN, SPI, HUB_SD_FREQ_HZ)) {
      if (SD.cardType() != CARD_NONE) {
        Serial.printf("[photos] microSD mounted on attempt %u: %.1f MB card\n",
                      static_cast<unsigned>(attempt),
                      static_cast<double>(SD.cardSize()) / (1024.0 * 1024.0));
        return true;
      }
      Serial.println("[photos] SD slot answered but holds no card");
      SD.end();
    }
    if (attempt < 3) delay(250);
  }

  Serial.println("[photos] microSD did NOT mount after 3 attempts.");
  Serial.printf("[photos]   check wiring: MISO=%u MOSI=%u SCK=%u CS=%u, card FAT32-formatted,\n"
                "[photos]   module VCC on 5V (regulated boards) or 3V3 (bare breakouts).\n",
                static_cast<unsigned>(HUB_SD_MISO_PIN), static_cast<unsigned>(HUB_SD_MOSI_PIN),
                static_cast<unsigned>(HUB_SD_SCK_PIN),  static_cast<unsigned>(HUB_SD_CS_PIN));
  Serial.println("[photos]   continuing on LittleFS -- photos still work, just not archived to the card.");
  SPI.end();
  return false;
}

bool PhotoStore::begin() {
  limit_ = GALLERY_MAX_PHOTOS < kMaxIndex ? GALLERY_MAX_PHOTOS : kMaxIndex;
  if (limit_ == 0) limit_ = 1;

  if (HUB_SD_ENABLED && mountSd()) {
    fs_      = &SD;
    usingSd_ = true;
  } else if (LittleFS.begin(true)) {
    fs_      = &LittleFS;
    usingSd_ = false;
    Serial.println("[photos] gallery stored in LittleFS (no SD card)");
  } else {
    Serial.println("[photos] ERROR: neither SD nor LittleFS is usable; gallery disabled");
    return false;
  }

  if (!fs_->exists(GALLERY_DIR) && !fs_->mkdir(GALLERY_DIR)) {
    Serial.printf("[photos] ERROR: cannot create %s; gallery disabled\n", GALLERY_DIR);
    fs_ = nullptr;
    return false;
  }

  // Somewhere to keep the mirrored seq floor when the gallery itself is on SD.
  if (usingSd_ && !quietExists(kLittleFsMount, GALLERY_DIR)) LittleFS.mkdir(GALLERY_DIR);

  discardUploadTemp();   // a crash mid-upload can leave the scratch file behind
  loadIndex();
  pruneOrphans();

  // Never hand out a seq below one already issued (see readSeqFloor).
  const uint16_t floorSeq = readSeqFloor();
  if (floorSeq > highestSeq_) highestSeq_ = floorSeq;

  Serial.printf("[photos] backend=%s  photos=%u/%u  newest seq=%u  used=%llu/%llu bytes\n",
                backend(), static_cast<unsigned>(count_), static_cast<unsigned>(limit_),
                static_cast<unsigned>(newestSeq()),
                static_cast<unsigned long long>(usedBytes()),
                static_cast<unsigned long long>(capacityBytes()));
  return true;
}

// ---------------------------------------------------------------------
//  Names
// ---------------------------------------------------------------------
void PhotoStore::buildPath(uint16_t seq, uint8_t station, char* out, size_t outLen) const {
  snprintf(out, outLen, "%s/%c%0*u%c%u%s",
           GALLERY_DIR, kNamePrefix, static_cast<int>(kSeqDigits),
           static_cast<unsigned>(seq), kStationMark,
           static_cast<unsigned>(station > 9 ? 9 : station), kNameExt);
}

// Accepts I<5 digits>S<1 digit>.JPG and rejects anything else, so a stray file
// on the card can never be served as a photo or deleted as an orphan.
bool PhotoStore::parseName(const char* name, uint16_t* seq, uint8_t* station) const {
  const char* p = baseName(name);
  if (*p++ != kNamePrefix) return false;

  uint32_t s = 0;
  int digits = 0;
  while (isdigit(static_cast<unsigned char>(*p)) && digits < kSeqDigits) {
    s = s * 10 + (*p++ - '0');
    digits++;
  }
  if (digits != kSeqDigits || s == 0 || s > 65535) return false;

  if (*p++ != kStationMark) return false;
  if (!isdigit(static_cast<unsigned char>(*p))) return false;
  const uint8_t st = static_cast<uint8_t>(*p++ - '0');

  if (strcasecmp(p, kNameExt) != 0) return false;

  *seq     = static_cast<uint16_t>(s);
  *station = st;
  return true;
}

// ---------------------------------------------------------------------
//  Sequence-number floor
//
//  /api/image?seq=N is served with a far-future immutable cache header, which
//  is only safe while a seq is never reused for different pixels. Scanning the
//  directory is not enough: "Clear photos" empties it, and the next capture
//  would then reuse seq 1 while every browser still has the OLD seq 1 cached --
//  the dashboard would show a stale photo, which is the exact bug this feature
//  exists to avoid. So the highest seq ever issued is kept in a one-line file
//  that clear() deliberately does not delete.
//
//  That file is written to BOTH the card and LittleFS, and the higher of the two
//  wins at boot, because the backend itself is not stable. An SPI card on jumper
//  wires can answer "Select Failed" on a boot it mounted fine on the last one
//  (see mountSd), which drops the gallery onto LittleFS; `uploadfs` rewrites
//  LittleFS from scratch; and cards get swapped. Any of those, with the floor
//  held only on the backend in use, restarts numbering at 1 -- and every browser
//  still holds a year-cached copy of the old seq 1, so the dashboard shows THAT
//  instead of the photo just captured.
// ---------------------------------------------------------------------
uint16_t PhotoStore::readSeqFloorFrom(fs::FS& fs, const char* mount) const {
  char path[48];
  snprintf(path, sizeof(path), "%s%s", GALLERY_DIR, kSeqName);
  if (!quietExists(mount, path)) return 0;

  File f = fs.open(path, "r");
  if (!f) return 0;

  char buf[8] = {0};
  const size_t n = f.readBytes(buf, sizeof(buf) - 1);
  f.close();
  buf[n] = 0;

  const long v = atol(buf);
  return (v > 0 && v <= 65535) ? static_cast<uint16_t>(v) : 0;
}

uint16_t PhotoStore::readSeqFloor() const {
  if (!fs_) return 0;
  uint16_t floorSeq = readSeqFloorFrom(LittleFS, kLittleFsMount);
  if (usingSd_) {
    const uint16_t onCard = readSeqFloorFrom(SD, kSdMount);
    if (onCard > floorSeq) floorSeq = onCard;
  }
  return floorSeq;
}

void PhotoStore::writeSeqFloorTo(fs::FS& fs, uint16_t seq) {
  char path[48];
  snprintf(path, sizeof(path), "%s%s", GALLERY_DIR, kSeqName);
  File f = fs.open(path, FILE_WRITE, true);
  if (!f) return;
  f.printf("%u\n", static_cast<unsigned>(seq));
  f.flush();
  f.close();
}

void PhotoStore::writeSeqFloor(uint16_t seq) {
  if (!fs_) return;
  writeSeqFloorTo(*fs_, seq);
  // Mirror onto flash so the floor outlives a card that fails to mount next boot.
  // LittleFS always holds the upload scratch, so it is already mounted here.
  if (usingSd_) writeSeqFloorTo(LittleFS, seq);
}

// ---------------------------------------------------------------------
//  Index maintenance
// ---------------------------------------------------------------------
void PhotoStore::insertNewest(const PhotoMeta& m) {
  // Descending by seq; the array is at most 32 entries so a shift is cheap.
  uint8_t pos = 0;
  while (pos < count_ && items_[pos].seq > m.seq) pos++;
  if (pos >= kMaxIndex) return;

  const uint8_t last = (count_ < kMaxIndex) ? count_ : static_cast<uint8_t>(kMaxIndex - 1);
  for (uint8_t i = last; i > pos; --i) items_[i] = items_[i - 1];
  items_[pos] = m;
  if (count_ < kMaxIndex) count_++;
  if (m.seq > highestSeq_) highestSeq_ = m.seq;
}

// Walk GALLERY_DIR once and keep the newest entries. Files that do not fit in
// the index are handled afterwards by pruneOrphans().
//
// Directories are iterated with getNextFileName(bool*) rather than
// openNextFile(): on the Arduino-ESP32 3.x core the File returned by
// openNextFile() carries a freed _path, so calling .name() on it reads dangling
// memory (the same crash documented in firmware/sdcard/src/main.cpp).
void PhotoStore::loadIndex() {
  count_      = 0;
  highestSeq_ = 0;
  if (!fs_) return;

  File dir = fs_->open(GALLERY_DIR);
  if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return; }

  bool isDir = false;
  for (;;) {
    String full = dir.getNextFileName(&isDir);   // FULL path, safe to read
    if (!full.length()) break;
    if (isDir) continue;

    PhotoMeta m;
    if (!parseName(full.c_str(), &m.seq, &m.station)) continue;

    File f = fs_->open(full.c_str(), "r");
    m.bytes = f ? static_cast<uint16_t>(f.size()) : 0;
    if (f) f.close();
    m.ms = 0;                            // unknown: this photo predates the boot
    insertNewest(m);
  }
  dir.close();

  if (count_ > limit_) count_ = limit_;  // keep only what the dashboard shows
}

// Delete every IMG_*.JPG that is not in items_. Names are collected before any
// removal because deleting mid-iteration is not safe on either filesystem.
void PhotoStore::pruneOrphans() {
  if (!fs_) return;

  const uint8_t kBatch = 8;
  for (;;) {
    char victims[kBatch][48];
    uint8_t nVictims = 0;

    File dir = fs_->open(GALLERY_DIR);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return; }

    bool isDir = false;
    while (nVictims < kBatch) {
      String full = dir.getNextFileName(&isDir);
      if (!full.length()) break;
      if (isDir) continue;

      uint16_t seq = 0;
      uint8_t  station = 0;
      if (parseName(full.c_str(), &seq, &station) && findSeq(seq) == nullptr) {
        snprintf(victims[nVictims], sizeof(victims[0]), "%s", full.c_str());
        nVictims++;
      }
    }
    dir.close();

    if (nVictims == 0) return;
    for (uint8_t i = 0; i < nVictims; ++i) {
      if (fs_->remove(victims[i])) Serial.printf("[photos] pruned %s\n", victims[i]);
      else                         return;   // cannot delete: stop, don't spin
    }
  }
}

void PhotoStore::evictOldest() {
  if (count_ == 0) return;
  const PhotoMeta& old = items_[count_ - 1];
  char path[48];
  buildPath(old.seq, old.station, path, sizeof(path));
  if (fs_ && fs_->exists(path)) fs_->remove(path);
  count_--;
}

// ---------------------------------------------------------------------
//  Upload lifecycle
// ---------------------------------------------------------------------
File PhotoStore::openUploadTemp() {
  if (!fs_) return File();
  discardUploadTemp();
  return LittleFS.open(kTempPath, FILE_WRITE, true);
}

File PhotoStore::openUploadTempForRead() const {
  if (!fs_ || !quietExists(kLittleFsMount, kTempPath)) return File();
  return LittleFS.open(kTempPath, "r");
}

void PhotoStore::discardUploadTemp() {
  if (quietExists(kLittleFsMount, kTempPath)) LittleFS.remove(kTempPath);
}

// Stream the validated scratch file onto the card. Runs at commit time only,
// never per HTTP chunk. Returns false (and cleans up) on any short write.
bool PhotoStore::copyTempTo(const char* dest, size_t bytes) {
  File src = LittleFS.open(kTempPath, "r");
  if (!src) return false;

  File dst = fs_->open(dest, FILE_WRITE, true);
  if (!dst) { src.close(); return false; }

  uint8_t buf[kCopyChunk];
  size_t copied = 0;
  bool ok = true;
  while (copied < bytes) {
    const int n = src.read(buf, sizeof(buf));
    if (n <= 0) { ok = false; break; }
    if (dst.write(buf, static_cast<size_t>(n)) != static_cast<size_t>(n)) { ok = false; break; }
    copied += static_cast<size_t>(n);
  }
  dst.flush();
  dst.close();
  src.close();

  if (!ok || copied != bytes) {
    Serial.printf("[photos] copy to %s failed: %u/%u bytes\n",
                  dest, static_cast<unsigned>(copied), static_cast<unsigned>(bytes));
    fs_->remove(dest);
    return false;
  }
  return true;
}

bool PhotoStore::commitUploadTemp(size_t bytes, uint8_t station, PhotoMeta* out) {
  if (!fs_ || bytes == 0) return false;
  if (!LittleFS.exists(kTempPath)) return false;

  // Make room BEFORE writing so a full card cannot lose the new photo.
  while (count_ >= limit_) evictOldest();

  // uint16_t seq would wrap after 65535 captures; wipe and restart instead so
  // the numbering stays strictly increasing. Unreachable in any demo run.
  if (highestSeq_ >= kSeqCeiling) {
    Serial.println("[photos] sequence ceiling reached -- wiping gallery and restarting at 1");
    while (count_ > 0) evictOldest();
    highestSeq_ = 0;
  }

  PhotoMeta m;
  m.seq     = static_cast<uint16_t>(highestSeq_ + 1);
  m.bytes   = static_cast<uint16_t>(bytes);
  m.station = station;
  m.ms      = millis();

  char dest[48];
  buildPath(m.seq, m.station, dest, sizeof(dest));
  // Never append into a leftover file; quiet-check first so the normal
  // "nothing there" case does not log a VFS error.
  if (quietExists(usingSd_ ? kSdMount : kLittleFsMount, dest)) fs_->remove(dest);

  if (usingSd_) {
    if (!copyTempTo(dest, bytes)) return false;  // scratch stays for a retry
    LittleFS.remove(kTempPath);
  } else if (!LittleFS.rename(kTempPath, dest)) {
    Serial.printf("[photos] ERROR: rename %s -> %s failed\n", kTempPath, dest);
    return false;
  }

  insertNewest(m);
  writeSeqFloor(highestSeq_);
  if (out) *out = m;
  return true;
}

// ---------------------------------------------------------------------
//  Reading
// ---------------------------------------------------------------------
const PhotoMeta* PhotoStore::findSeq(uint16_t seq) const {
  for (uint8_t i = 0; i < count_; ++i) if (items_[i].seq == seq) return &items_[i];
  return nullptr;
}

const PhotoMeta* PhotoStore::at(uint8_t index) const {
  return (index < count_) ? &items_[index] : nullptr;
}

File PhotoStore::openSeq(uint16_t seq) const {
  const PhotoMeta* m = findSeq(seq);
  if (!m || !fs_) return File();
  char path[48];
  buildPath(m->seq, m->station, path, sizeof(path));
  return fs_->open(path, "r");
}

// ---------------------------------------------------------------------
//  Housekeeping
// ---------------------------------------------------------------------
void PhotoStore::clear() {
  if (!fs_) return;
  while (count_ > 0) evictOldest();
  discardUploadTemp();
  pruneOrphans();
  // highestSeq_ is deliberately kept: the next photo must NOT reuse a seq that
  // a browser already cached as immutable (see readSeqFloor).
  Serial.printf("[photos] gallery cleared; next seq = %u\n",
                static_cast<unsigned>(highestSeq_ + 1));
}

uint64_t PhotoStore::capacityBytes() const {
  if (!fs_) return 0;
  return usingSd_ ? SD.totalBytes() : LittleFS.totalBytes();
}

uint64_t PhotoStore::usedBytes() const {
  if (!fs_) return 0;
  return usingSd_ ? SD.usedBytes() : LittleFS.usedBytes();
}
