#pragma once
// =====================================================================
//  MuseumProtocol.h  —  the one ESP-NOW packet every node uses.
//  Identical across the Hub, robot, health, and emergency nodes, so they
//  all agree on the layout. Keep it in sync with the other nodes.
// =====================================================================

#include <Arduino.h>
#include <stddef.h>

namespace museum {

constexpr uint8_t kEspNowChannel = 1;   // MUST match the Hub SoftAP channel

constexpr uint16_t kMessageMagic = 0x4D53;   // "MS"
constexpr uint8_t  kProtocolVersion = 1;

constexpr uint8_t kBroadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

enum MessageType : uint8_t {
  MSG_LOCATION = 1, MSG_EMERGENCY = 2, MSG_HEALTH = 3,
  MSG_VISITOR_COUNT = 4, MSG_IMAGE_READY = 5, MSG_COMMAND = 6, MSG_HEARTBEAT = 7,
  // Hub -> robot: the admin dashboard's mute switch. Deliberately its OWN type
  // rather than a flag riding on MSG_EMERGENCY: the physical emergency-button
  // node also broadcasts MSG_EMERGENCY and knows nothing about the mute, so its
  // packets would clear it on every press.
  MSG_AUDIO = 8,
};

enum MessageFlag : uint8_t {
  FLAG_NONE = 0,
  FLAG_EMERGENCY_ACTIVE = 1 << 0,
  FLAG_OBSTACLE_DETECTED = 1 << 1,
  FLAG_ALERT_ACTIVE = 1 << 2,
  FLAG_DEVICE_ONLINE = 1 << 3,
  FLAG_ROBOT_MOVING = 1 << 4,
  FLAG_AUDIO_MUTED = 1 << 5,      // MSG_AUDIO: narration volume forced to 0
};

// Fixed-width integers (no float) so every ESP32 agrees on the layout.
// 26.4 C is stored as tempX10 = 264.
struct __attribute__((packed)) MuseumMessage {
  uint16_t magic;
  uint8_t  version;
  uint8_t  messageType;
  uint8_t  nodeId;
  uint8_t  stationId;     // 1..N (0 = none)
  uint8_t  flags;
  uint8_t  reserved;
  int16_t  tempX10;
  int16_t  humidX10;
  uint16_t gasRaw;
  uint16_t visitorCount;
  uint16_t sequence;
  uint32_t uptimeMs;
  uint16_t checksum;
};

inline uint16_t calculateChecksum(const MuseumMessage &m) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(&m);
  const size_t dataLength = offsetof(MuseumMessage, checksum);
  uint16_t sum = 0;
  for (size_t i = 0; i < dataLength; ++i) sum = static_cast<uint16_t>(sum + bytes[i]);
  return sum;
}

inline void finalizeMessage(MuseumMessage &m) {
  m.magic = kMessageMagic;
  m.version = kProtocolVersion;
  m.checksum = calculateChecksum(m);
}

inline MuseumMessage makeMessage(MessageType type, uint8_t nodeId) {
  MuseumMessage m{};
  m.messageType = static_cast<uint8_t>(type);
  m.nodeId = nodeId;
  finalizeMessage(m);
  return m;
}

inline bool isValidMessage(const MuseumMessage &m) {
  return m.magic == kMessageMagic &&
         m.version == kProtocolVersion &&
         m.checksum == calculateChecksum(m);
}

}  // namespace museum
