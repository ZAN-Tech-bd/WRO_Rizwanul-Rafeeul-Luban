// =====================================================================
//  ble_ota.cpp  —  see include/ble_ota.h for the contract.
//
//  Custom GATT OTA service (NimBLE). A host (tools/ble_ota_upload.py) streams
//  a firmware.bin into the inactive OTA partition; on FINISH we commit and
//  reboot into it. Every session must present BLE_OTA_PASSWORD (config.h) via
//  an AUTH command before BEGIN/DATA are accepted -- see the note on
//  BLE_OTA_PASSWORD in config.h for what this password does and does not
//  protect against. Rotate the UUIDs too if you want to "change the lock".
// =====================================================================

#include "ble_ota.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string.h>

#include "config.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

namespace {

// ---- custom 128-bit GATT UUIDs (random) ----
const char* UUID_SERVICE = "8e7f1a01-9b3c-4e2a-8d11-7c0e9f3a2b10";
const char* UUID_CMD     = "8e7f1a02-9b3c-4e2a-8d11-7c0e9f3a2b10";  // WRITE: control
const char* UUID_DATA    = "8e7f1a03-9b3c-4e2a-8d11-7c0e9f3a2b10";  // WRITE: firmware bytes
const char* UUID_STATUS  = "8e7f1a04-9b3c-4e2a-8d11-7c0e9f3a2b10";  // READ+NOTIFY: state

// CMD opcodes (first byte of a CMD write).
const uint8_t OP_AUTH   = 0x00;   // followed by the UTF-8 password bytes
const uint8_t OP_BEGIN  = 0x01;   // followed by a 4-byte big-endian total size
const uint8_t OP_FINISH = 0x02;   // commit + reboot
const uint8_t OP_ABORT  = 0x03;   // cancel

// STATUS states (first byte of the STATUS value).
const uint8_t ST_IDLE     = 0x00;
const uint8_t ST_AUTH_OK  = 0x01;
const uint8_t ST_WRITING  = 0x02;
const uint8_t ST_DONE     = 0x03;
const uint8_t ST_ERROR    = 0xFF;

// A wrong password is cheap to guess if left unlimited (there is no pairing
// delay gating attempts). Disconnect the link after too many misses in one
// connection so a script can't just spin through a wordlist.
const uint8_t MAX_AUTH_FAILS = 5;

// ---- OTA session state (touched only from the NimBLE host task) ----
volatile bool         gInProgress   = false;
volatile uint32_t     gLastActivityMs = 0;   // millis() of the last BEGIN/DATA write
uint32_t              gTotal        = 0;
uint32_t              gWritten      = 0;
uint8_t               gState        = ST_IDLE;
esp_ota_handle_t      gHandle       = 0;
const esp_partition_t* gPart        = nullptr;
bool                   gAuthenticated = false;
uint8_t                gAuthFails     = 0;

NimBLEServer*          gServer   = nullptr;
NimBLECharacteristic*  gStatusCh = nullptr;

// Constant-time compare so a wrong-password reply can't be timed byte-by-byte.
bool passwordMatches(const uint8_t* given, size_t givenLen) {
  const char* want = BLE_OTA_PASSWORD;
  size_t wantLen = strlen(want);
  if (givenLen != wantLen) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < wantLen; i++) {
    diff |= given[i] ^ static_cast<uint8_t>(want[i]);
  }
  return diff == 0;
}

void notifyStatus() {
  if (!gStatusCh) return;
  uint8_t buf[5] = {
    gState,
    static_cast<uint8_t>((gWritten >> 24) & 0xFF),
    static_cast<uint8_t>((gWritten >> 16) & 0xFF),
    static_cast<uint8_t>((gWritten >> 8) & 0xFF),
    static_cast<uint8_t>(gWritten & 0xFF)
  };
  gStatusCh->setValue(buf, sizeof(buf));
  gStatusCh->notify();
}

void failOta(const char* msg) {
  Serial.printf("[ble-ota] %s (written=%u/%u)\n", msg, gWritten, gTotal);
  if (gHandle && gPart) esp_ota_abort(gHandle);
  gHandle = 0; gPart = nullptr;
  gState = ST_ERROR; gInProgress = false;
  notifyStatus();
}

// A dropped BLE link (weak signal, a laptop's Bluetooth stack hiccup, etc.)
// commonly lands right after the last DATA byte but before the client's
// FINISH ever arrives -- a full multi-minute transfer must not be thrown
// away for want of one more write. If every promised byte is already in
// place, finish the commit ourselves; only abort a genuinely partial write.
void finalizeOrAbortPending() {
  if (gState != ST_WRITING || !gHandle || !gPart) return;
  if (gTotal > 0 && gWritten == gTotal) {
    Serial.println("[ble-ota] link dropped right after 100% write -- auto-committing");
    esp_ota_handle_t handle = gHandle;
    const esp_partition_t* part = gPart;
    gHandle = 0; gPart = nullptr;
    esp_err_t e = esp_ota_end(handle);
    if (e == ESP_OK) e = esp_ota_set_boot_partition(part);
    if (e == ESP_OK) {
      Serial.printf("[ble-ota] auto-commit ok -> reboot into %s\n", part->label);
      delay(200);
      esp_restart();
      return;  // unreachable
    }
    Serial.printf("[ble-ota] auto-commit failed (%d) -- image discarded\n", (int)e);
    return;
  }
  Serial.printf("[ble-ota] link dropped mid-transfer (written=%u/%u) -- aborting\n", gWritten, gTotal);
  esp_ota_abort(gHandle);
  gHandle = 0; gPart = nullptr;
}

// Resets everything an active/pending session was holding. Used both on
// disconnect and after a password lockout, so a new connection always starts
// from a clean, unauthenticated slate.
void resetSession() {
  finalizeOrAbortPending();  // may reboot and never return
  gHandle = 0; gPart = nullptr;
  gInProgress = false;
  gAuthenticated = false;
  gAuthFails = 0;
  gState = ST_IDLE;
}

// ---- control characteristic ----
class CmdCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, ble_gap_conn_desc* desc) override {
    std::string v = pChar->getValue();
    if (v.size() < 1) return;
    const uint8_t* b = reinterpret_cast<const uint8_t*>(v.data());

    switch (b[0]) {
      case OP_AUTH: {
        bool ok = passwordMatches(b + 1, v.size() - 1);
        if (ok) {
          gAuthenticated = true;
          gAuthFails = 0;
          gState = ST_AUTH_OK;
          Serial.println("[ble-ota] AUTH ok");
          notifyStatus();
        } else {
          gAuthenticated = false;
          gAuthFails++;
          gState = ST_ERROR;
          Serial.printf("[ble-ota] AUTH failed (%u/%u attempts)\n", gAuthFails, MAX_AUTH_FAILS);
          notifyStatus();
          if (gAuthFails >= MAX_AUTH_FAILS && gServer && desc) {
            Serial.println("[ble-ota] too many failed AUTH attempts -- disconnecting");
            gServer->disconnect(desc->conn_handle);
          }
        }
        return;
      }
      case OP_BEGIN: {
        if (!gAuthenticated) { failOta("BEGIN without AUTH"); return; }
        if (v.size() < 5) { failOta("BEGIN missing size"); return; }
        gTotal   = ((uint32_t)b[1] << 24) | ((uint32_t)b[2] << 16) |
                   ((uint32_t)b[3] << 8)  |  (uint32_t)b[4];
        gWritten = 0;
        gPart    = esp_ota_get_next_update_partition(nullptr);
        if (!gPart) { failOta("no update partition"); return; }
        esp_err_t e = esp_ota_begin(gPart, gTotal, &gHandle);
        if (e != ESP_OK) { failOta("esp_ota_begin failed"); return; }
        gState = ST_WRITING; gInProgress = true; gLastActivityMs = millis();
        Serial.printf("[ble-ota] BEGIN size=%u -> partition %s\n", gTotal, gPart->label);
        notifyStatus();
        break;
      }
      case OP_FINISH: {
        if (gState != ST_WRITING) { failOta("FINISH without BEGIN"); return; }
        if (gWritten != gTotal)   { failOta("size mismatch at FINISH"); return; }
        esp_err_t e = esp_ota_end(gHandle);
        gHandle = 0;
        if (e != ESP_OK) { failOta("esp_ota_end failed"); return; }
        e = esp_ota_set_boot_partition(gPart);
        if (e != ESP_OK) { failOta("set_boot_partition failed"); return; }
        gState = ST_DONE; notifyStatus();
        Serial.printf("[ble-ota] DONE %u bytes -> reboot into %s\n", gWritten, gPart->label);
        // Give the BLE stack a moment to push the final notify + disconnect.
        delay(400);
        esp_restart();
        break;  // unreachable
      }
      case OP_ABORT: {
        failOta("ABORT by host");
        return;
      }
      default:
        Serial.printf("[ble-ota] unknown opcode 0x%02X\n", b[0]);
    }
  }
};

// ---- data characteristic: each write is one firmware chunk ----
class DataCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar) override {
    // gState only reaches ST_WRITING via an authenticated BEGIN, so this
    // implicitly rejects data from a session that never sent AUTH.
    if (gState != ST_WRITING || !gHandle) return;   // ignore data outside a session
    std::string v = pChar->getValue();
    if (v.empty()) return;
    esp_err_t e = esp_ota_write(gHandle, v.data(), v.size());
    if (e != ESP_OK) { failOta("esp_ota_write failed"); return; }
    gWritten += v.size();
    gLastActivityMs = millis();

    static uint32_t lastReport = 0;
    if (gWritten - lastReport >= 65536 || gWritten == gTotal) {
      lastReport = gWritten;
      Serial.printf("[ble-ota] %u/%u (%u%%)\n", gWritten, gTotal,
                    gTotal ? (gWritten * 100 / gTotal) : 0);
      notifyStatus();
    }
  }
};

// A new connection must re-authenticate, and a dropped connection must not
// leave a half-written OTA partition dangling for the next client.
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
    resetSession();
    Serial.println("[ble-ota] client connected -- send AUTH to unlock BEGIN/DATA");
  }
  void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
    resetSession();
    Serial.println("[ble-ota] client disconnected -- session reset");
  }
};

}  // namespace

void bleOtaBegin() {
  NimBLEDevice::init("MuseumHub-BLE-OTA");
  NimBLEDevice::setMTU(512);

  NimBLEServer*   server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  gServer = server;
  NimBLEService*  svc    = server->createService(UUID_SERVICE);

  NimBLECharacteristic* cmdCh =
      svc->createCharacteristic(UUID_CMD, NIMBLE_PROPERTY::WRITE);
  cmdCh->setCallbacks(new CmdCallbacks());

  NimBLECharacteristic* dataCh =
      svc->createCharacteristic(UUID_DATA, NIMBLE_PROPERTY::WRITE);
  dataCh->setCallbacks(new DataCallbacks());

  gStatusCh = svc->createCharacteristic(UUID_STATUS,
                                        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  uint8_t zero[5] = {0, 0, 0, 0, 0};
  gStatusCh->setValue(zero, sizeof(zero));

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_SERVICE);
  adv->start();

  Serial.println("[ble-ota] advertising OTA service (NimBLE). "
                 "Flash with tools/ble_ota_upload.py <firmware.bin>");
}

bool bleOtaInProgress() {
  if (!gInProgress) return false;
  // Read once: the NimBLE host task writes this concurrently.
  const uint32_t last = gLastActivityMs;
  // An abandoned upload must never keep the emergency button parked. Report
  // "not in progress" once the stream goes quiet; the session state itself is
  // left alone so a resuming client (or the disconnect handler) still tidies
  // up correctly, and a real upload re-parks Core 1 on its next chunk.
  if ((uint32_t)(millis() - last) > BLE_OTA_STALL_MS) return false;
  return true;
}
