#pragma once
// =====================================================================
//  PhotoStore.h  —  the Hub's camera-photo gallery on persistent storage.
//
//  Every JPEG the ESP32-CAM uploads is written to GALLERY_DIR as
//  IMG_<seq>_S<station>.JPG, so both the sequence number and the station
//  survive a reboot without any sidecar index file.
//
//  Backend choice (decided once, in begin()):
//    1. microSD over SPI  — preferred; wiring in config.h (HUB_SD_* pins).
//    2. LittleFS          — automatic fallback when no card is present, so the
//                           dashboard still works on a Hub with no SD module.
//
//  The store keeps the newest GALLERY_MAX_PHOTOS files and deletes the rest,
//  which bounds both the card usage and the RAM index below.
//
//  Threading: every method is called from the web task (Core 0) only. It does
//  NOT take gStateMutex — callers update gState themselves.
// =====================================================================

#include <Arduino.h>
#include <FS.h>

// One entry of the in-RAM index. items[0] is always the newest photo.
struct PhotoMeta {
  uint16_t seq     = 0;   // monotonic capture number (1, 2, 3, ...)
  uint16_t bytes   = 0;   // JPEG size on disk
  uint8_t  station = 0;   // 1..STATION_COUNT (0 = unknown)
  uint32_t ms      = 0;   // millis() when it arrived; 0 = from an earlier boot
};

class PhotoStore {
 public:
  // Mount a backend and load the index. Returns false only if BOTH the card and
  // LittleFS are unusable (the Hub then runs without a gallery).
  bool begin();

  bool        ready()    const { return fs_ != nullptr; }
  bool        usingSd()  const { return usingSd_; }
  const char* backend()  const { return !fs_ ? "none" : (usingSd_ ? "sd" : "littlefs"); }

  // ---- upload lifecycle (one at a time) ----
  //
  // The scratch file ALWAYS lives in LittleFS, never on the card, even when the
  // gallery itself is on SD. The upload callback runs inside the WebServer's
  // body-parsing loop, and putting per-chunk SPI writes there made uploads
  // truncate (the Hub then answered 422 invalid_jpeg). Flash writes are fast and
  // the card is touched once, at commit, outside that loop.
  File openUploadTemp();
  File openUploadTempForRead() const;    // used to validate the JPEG markers
  void discardUploadTemp();
  // Move the scratch file into the gallery, evict the oldest photo if the limit
  // is reached, and fill *out with the committed entry.
  bool commitUploadTemp(size_t bytes, uint8_t station, PhotoMeta* out);

  // ---- reading ----
  File             openSeq(uint16_t seq) const;   // invalid File if unknown
  const PhotoMeta* findSeq(uint16_t seq) const;   // nullptr if unknown
  const PhotoMeta* at(uint8_t index) const;       // 0 = newest, nullptr past end
  uint8_t          count()  const { return count_; }
  uint16_t         newestSeq() const { return count_ ? items_[0].seq : 0; }

  // ---- housekeeping ----
  void     clear();                 // delete every photo and reset the index
  uint64_t capacityBytes() const;
  uint64_t usedBytes()     const;

 private:
  static const uint8_t kMaxIndex = 32;   // hard cap; GALLERY_MAX_PHOTOS <= this

  fs::FS*   fs_        = nullptr;
  bool      usingSd_   = false;
  uint8_t   limit_     = 0;             // effective GALLERY_MAX_PHOTOS
  PhotoMeta items_[kMaxIndex];
  uint8_t   count_     = 0;
  uint16_t  highestSeq_ = 0;            // survives eviction, so seq never repeats

  bool mountSd();
  bool copyTempTo(const char* dest, size_t bytes);   // LittleFS scratch -> card
  uint16_t readSeqFloor() const;        // highest seq ever issued, from SEQ.TXT
  void     writeSeqFloor(uint16_t seq);
  void buildPath(uint16_t seq, uint8_t station, char* out, size_t outLen) const;
  bool parseName(const char* name, uint16_t* seq, uint8_t* station) const;
  void loadIndex();                     // scan GALLERY_DIR into items_
  void insertNewest(const PhotoMeta& m);
  void evictOldest();                   // drop items_[count_-1] and its file
  void pruneOrphans();                  // delete files that fell out of items_
};
