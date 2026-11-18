#include "BleHub.h"

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <atomic>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>

#include "OtaPublicKey.h"

namespace {

constexpr char SERVICE_UUID[] = "8e7f1a01-9b3c-4e2a-8d11-7c0e9f3a2b10";
constexpr char COMMAND_UUID[] = "8e7f1a02-9b3c-4e2a-8d11-7c0e9f3a2b10";
constexpr char DATA_UUID[] = "8e7f1a03-9b3c-4e2a-8d11-7c0e9f3a2b10";
constexpr char STATUS_UUID[] = "8e7f1a04-9b3c-4e2a-8d11-7c0e9f3a2b10";
constexpr char CHALLENGE_UUID[] = "8e7f1a05-9b3c-4e2a-8d11-7c0e9f3a2b10";
constexpr char DIAGNOSTICS_UUID[] = "8e7f1a06-9b3c-4e2a-8d11-7c0e9f3a2b10";

constexpr uint8_t BEGIN = 1, FINISH = 2, ABORT = 3;
constexpr uint8_t IDLE = 0, WRITING = 2, DONE = 3, ERROR = 255;
enum Error : uint8_t { None, Command, Signature, ImageSize, Flash, Digest, Timeout };
constexpr size_t NONCE_SIZE = 16;
constexpr size_t SHA256_SIZE = 32;
constexpr size_t MANIFEST_SIZE = 1 + 4 + SHA256_SIZE; // opcode, big-endian size, image digest
constexpr uint32_t TRANSFER_TIMEOUT_MS = 15000;

NimBLECharacteristic* status = nullptr;
NimBLECharacteristic* challenge = nullptr;
NimBLECharacteristic* diagnostics = nullptr;

// Transfer state is owned by the NimBLE host task. Only the delayed restart
// request crosses to the Arduino loop, which continues running during writes.
std::atomic<uint32_t> committedAt{0};
std::atomic<bool> transferActive{false};
bool (*allowUpdate)() = nullptr;
uint8_t nonce[NONCE_SIZE];
mbedtls_pk_context publicKey;
mbedtls_sha256_context imageHash;
uint8_t expectedHash[SHA256_SIZE];
const esp_partition_t* target = nullptr;
esp_ota_handle_t handle = 0;
uint32_t total = 0, written = 0, lastData = 0;

void setStatus(uint8_t state, Error error = None) {
  const uint8_t value[] = {state, uint8_t(written >> 24), uint8_t(written >> 16),
                           uint8_t(written >> 8), uint8_t(written), uint8_t(error)};
  status->setValue(value, sizeof(value));
}

void newChallenge() {
  esp_fill_random(nonce, sizeof(nonce));
  challenge->setValue(nonce, sizeof(nonce));
}

void abortTransfer() {
  if (handle) {
    esp_ota_abort(handle);
    handle = 0;
    mbedtls_sha256_free(&imageHash);
  }
  transferActive.store(false);
}

void fail(Error error) {
  abortTransfer();
  setStatus(ERROR, error);
  Serial.printf("[ble-ota] rejected (%u), %lu/%lu bytes\n", unsigned(error),
                static_cast<unsigned long>(written), static_cast<unsigned long>(total));
}

bool transferExpired() {
  if (!handle || millis() - lastData < TRANSFER_TIMEOUT_MS) return false;
  fail(Timeout);
  return true;
}

void beginTransfer(const uint8_t* data, size_t length) {
  if (handle || length <= MANIFEST_SIZE || length > MANIFEST_SIZE + 72) {
    fail(Command);
    return;
  }

  uint8_t signedData[NONCE_SIZE + MANIFEST_SIZE];
  memcpy(signedData, nonce, NONCE_SIZE);
  memcpy(signedData + NONCE_SIZE, data, MANIFEST_SIZE);
  newChallenge();

  uint8_t manifestHash[SHA256_SIZE];
  if (mbedtls_sha256_ret(signedData, sizeof(signedData), manifestHash, 0) != 0 ||
      mbedtls_pk_verify(&publicKey, MBEDTLS_MD_SHA256, manifestHash, SHA256_SIZE,
                        data + MANIFEST_SIZE, length - MANIFEST_SIZE) != 0) {
    fail(Signature);
    return;
  }

  written = 0;
  total = (uint32_t(data[1]) << 24) | (uint32_t(data[2]) << 16) |
          (uint32_t(data[3]) << 8) | uint32_t(data[4]);
  target = esp_ota_get_next_update_partition(nullptr);
  if (!target || total == 0 || total > target->size) {
    fail(ImageSize);
    return;
  }

  memcpy(expectedHash, data + 5, SHA256_SIZE);
  // Close the motion gate before checking it so the robot loop also sees the
  // transfer request while the ESP-NOW and Arduino tasks are running.
  transferActive.store(true);
  if (allowUpdate && !allowUpdate()) {
    fail(Command);
    return;
  }
  if (esp_ota_begin(target, OTA_WITH_SEQUENTIAL_WRITES, &handle) != ESP_OK) {
    handle = 0;
    fail(Flash);
    return;
  }
  mbedtls_sha256_init(&imageHash);
  if (mbedtls_sha256_starts_ret(&imageHash, 0) != 0) {
    fail(Digest);
    return;
  }
  lastData = millis();
  setStatus(WRITING);
  Serial.printf("[ble-ota] writing %lu bytes to %s\n", static_cast<unsigned long>(total), target->label);
}

void finishTransfer() {
  if (!handle || written != total) {
    fail(ImageSize);
    return;
  }

  uint8_t actualHash[SHA256_SIZE];
  if (mbedtls_sha256_finish_ret(&imageHash, actualHash) != 0 ||
      memcmp(actualHash, expectedHash, SHA256_SIZE) != 0) {
    fail(Digest);
    return;
  }

  mbedtls_sha256_free(&imageHash);
  const esp_err_t verified = esp_ota_end(handle);
  handle = 0;
  if (verified != ESP_OK || esp_ota_set_boot_partition(target) != ESP_OK) {
    fail(Flash);
    return;
  }
  setStatus(DONE);
  Serial.printf("[ble-ota] verified image committed to %s\n", target->label);
  committedAt.store(millis());
  transferActive.store(false);
}

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic) override {
    if (committedAt.load()) return;
    const std::string value = characteristic->getValue();
    if (value.empty()) return;
    const auto* data = reinterpret_cast<const uint8_t*>(value.data());
    if (transferExpired() && data[0] != BEGIN) return;
    if (data[0] == BEGIN) {
      beginTransfer(data, value.size());
    } else if (data[0] == FINISH && value.size() == 1) {
      finishTransfer();
    } else if (data[0] == ABORT && value.size() == 1) {
      abortTransfer();
      written = 0;
      setStatus(IDLE);
    } else {
      fail(Command);
    }
  }
};

class DataCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic) override {
    if (!handle || transferExpired()) return;
    const std::string value = characteristic->getValue();
    if (value.empty() || value.size() > total - written) {
      fail(ImageSize);
      return;
    }
    if (esp_ota_write(handle, value.data(), value.size()) != ESP_OK) {
      fail(Flash);
      return;
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(value.data());
    if (mbedtls_sha256_update_ret(&imageHash, bytes, value.size()) != 0) {
      fail(Digest);
      return;
    }
    written += value.size();
    lastData = millis();
  }
};

class StatusCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic*) override {
    if (transferExpired()) return;
    if (handle) setStatus(WRITING);
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, ble_gap_conn_desc* connection) override {
    newChallenge();
    server->updateConnParams(connection->conn_handle, 12, 24, 0, 200);
  }

  void onDisconnect(NimBLEServer*) override {
    if (handle) {
      Serial.printf("[ble-ota] disconnected after %lu/%lu bytes; update discarded\n",
                    static_cast<unsigned long>(written), static_cast<unsigned long>(total));
      abortTransfer();
    }
    if (!committedAt.load()) {
      written = 0;
      setStatus(IDLE);
    }
  }
};

CommandCallbacks commandCallbacks;
DataCallbacks dataCallbacks;
StatusCallbacks statusCallbacks;
ServerCallbacks serverCallbacks;

}  // namespace

bool bleHubBegin(const char* deviceName, bool (*canUpdate)()) {
  allowUpdate = canUpdate;
  mbedtls_pk_init(&publicKey);
  if (mbedtls_pk_parse_public_key(&publicKey, reinterpret_cast<const uint8_t*>(OTA_PUBLIC_KEY),
                                  sizeof(OTA_PUBLIC_KEY)) != 0) {
    Serial.println("[ble-ota] invalid public key");
    return false;
  }

  NimBLEDevice::init(deviceName);
  NimBLEDevice::setMTU(512);
  auto* server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  server->advertiseOnDisconnect(true);
  auto* service = server->createService(SERVICE_UUID);
  service->createCharacteristic(COMMAND_UUID, NIMBLE_PROPERTY::WRITE, 128)->setCallbacks(&commandCallbacks);
  service->createCharacteristic(DATA_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR, 512)
      ->setCallbacks(&dataCallbacks);
  status = service->createCharacteristic(STATUS_UUID, NIMBLE_PROPERTY::READ, 6);
  status->setCallbacks(&statusCallbacks);
  challenge = service->createCharacteristic(CHALLENGE_UUID, NIMBLE_PROPERTY::READ, NONCE_SIZE);
  diagnostics = service->createCharacteristic(DIAGNOSTICS_UUID, NIMBLE_PROPERTY::READ,
                                              BLE_HUB_DIAGNOSTICS_CAPACITY);
  setStatus(IDLE);
  // Reserve before advertising so a longer snapshot cannot reallocate the
  // characteristic buffer while the NimBLE host is reading it on Core 0.
  const uint8_t reserved[BLE_HUB_DIAGNOSTICS_CAPACITY - 1] = {};
  diagnostics->setValue(reserved, sizeof(reserved));
  bleHubSetDiagnostics("NOV_HUB starting");
  if (!service->start()) return false;

  auto* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  return advertising->start();
}

void bleHubSetDiagnostics(const char* line) {
  if (diagnostics) diagnostics->setValue(reinterpret_cast<const uint8_t*>(line), strlen(line));
}

bool bleHubTransferActive() {
  return transferActive.load() || committedAt.load() != 0;
}

void bleHubPoll(bool alarmOrButtonActive) {
  const uint32_t timestamp = committedAt.load();
  if (timestamp && !alarmOrButtonActive && millis() - timestamp >= 1500) esp_restart();
}
