#pragma once

#include <Arduino.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace robot_uart {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint32_t kBaud = 9600;
constexpr size_t kMaxUidBytes = 10;
constexpr size_t kMaxUidHexLength = kMaxUidBytes * 2U;
constexpr size_t kUidBufferSize = kMaxUidHexLength + 1U;
constexpr size_t kMaxFrameLength = 72;

struct RfidEvent {
  uint16_t sequence = 0;
  char uidHex[kUidBufferSize]{};
  uint8_t sak = 0;
  uint8_t piccType = 0;
};

inline uint8_t xorChecksum(const char* data, const size_t length) {
  uint8_t checksum = 0;
  for (size_t index = 0; index < length; ++index) {
    checksum ^= static_cast<uint8_t>(data[index]);
  }
  return checksum;
}

inline int8_t hexNibble(const char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}

inline bool parseHexByte(const char* text, uint8_t& value) {
  if (text == nullptr || text[0] == '\0' || text[1] == '\0' ||
      text[2] != '\0') {
    return false;
  }
  const int8_t high = hexNibble(text[0]);
  const int8_t low = hexNibble(text[1]);
  if (high < 0 || low < 0) return false;
  value = static_cast<uint8_t>((high << 4) | low);
  return true;
}

inline bool parseUnsigned(const char* text, const unsigned long maximum,
                          unsigned long& value) {
  if (text == nullptr || *text == '\0') return false;
  char* end = nullptr;
  const unsigned long parsed = strtoul(text, &end, 10);
  if (*end != '\0' || parsed > maximum) return false;
  value = parsed;
  return true;
}

inline bool normalizeUid(const char* source, char* destination,
                         const size_t capacity) {
  if (source == nullptr || destination == nullptr || capacity == 0U) {
    return false;
  }
  const size_t length = strlen(source);
  if (length < 2U || length > kMaxUidHexLength || (length % 2U) != 0U ||
      length + 1U > capacity) {
    return false;
  }
  for (size_t index = 0; index < length; ++index) {
    if (hexNibble(source[index]) < 0) return false;
    destination[index] = static_cast<char>(toupper(source[index]));
  }
  destination[length] = '\0';
  return true;
}

// RFID|1|<sequence>|<UID_HEX>|<SAK_HEX>|<PICC_TYPE>|<XOR_HEX>\n
inline size_t encodeRfidFrame(const RfidEvent& event, char* output,
                              const size_t capacity) {
  char normalizedUid[kUidBufferSize]{};
  if (output == nullptr || capacity == 0U ||
      !normalizeUid(event.uidHex, normalizedUid, sizeof(normalizedUid))) {
    return 0U;
  }

  const int prefixLength = snprintf(
      output, capacity, "RFID|%u|%u|%s|%02X|%u",
      static_cast<unsigned int>(kProtocolVersion),
      static_cast<unsigned int>(event.sequence), normalizedUid,
      static_cast<unsigned int>(event.sak),
      static_cast<unsigned int>(event.piccType));
  if (prefixLength <= 0 || static_cast<size_t>(prefixLength) >= capacity) {
    return 0U;
  }

  const uint8_t checksum =
      xorChecksum(output, static_cast<size_t>(prefixLength));
  const int suffixLength = snprintf(
      output + prefixLength, capacity - static_cast<size_t>(prefixLength),
      "|%02X\n", static_cast<unsigned int>(checksum));
  if (suffixLength <= 0 ||
      static_cast<size_t>(prefixLength + suffixLength) >= capacity) {
    return 0U;
  }
  return static_cast<size_t>(prefixLength + suffixLength);
}

inline bool parseRfidFrame(char* line, RfidEvent& event) {
  if (line == nullptr) return false;

  char* checksumSeparator = strrchr(line, '|');
  if (checksumSeparator == nullptr) return false;
  uint8_t suppliedChecksum = 0;
  if (!parseHexByte(checksumSeparator + 1, suppliedChecksum)) return false;
  const size_t payloadLength = static_cast<size_t>(checksumSeparator - line);
  if (xorChecksum(line, payloadLength) != suppliedChecksum) return false;
  *checksumSeparator = '\0';

  char* save = nullptr;
  const char* kind = strtok_r(line, "|", &save);
  const char* versionText = strtok_r(nullptr, "|", &save);
  const char* sequenceText = strtok_r(nullptr, "|", &save);
  const char* uidText = strtok_r(nullptr, "|", &save);
  const char* sakText = strtok_r(nullptr, "|", &save);
  const char* typeText = strtok_r(nullptr, "|", &save);
  const char* extra = strtok_r(nullptr, "|", &save);
  if (kind == nullptr || strcmp(kind, "RFID") != 0 || extra != nullptr) {
    return false;
  }

  unsigned long version = 0;
  unsigned long sequence = 0;
  unsigned long piccType = 0;
  uint8_t sak = 0;
  RfidEvent parsed{};
  if (!parseUnsigned(versionText, UINT8_MAX, version) ||
      version != kProtocolVersion ||
      !parseUnsigned(sequenceText, UINT16_MAX, sequence) || sequence == 0U ||
      !normalizeUid(uidText, parsed.uidHex, sizeof(parsed.uidHex)) ||
      !parseHexByte(sakText, sak) ||
      !parseUnsigned(typeText, UINT8_MAX, piccType)) {
    return false;
  }

  parsed.sequence = static_cast<uint16_t>(sequence);
  parsed.sak = sak;
  parsed.piccType = static_cast<uint8_t>(piccType);
  event = parsed;
  return true;
}

inline size_t encodeAck(const uint16_t sequence, char* output,
                        const size_t capacity) {
  if (output == nullptr || capacity == 0U || sequence == 0U) return 0U;
  const int length = snprintf(output, capacity, "ACK|%u\n",
                              static_cast<unsigned int>(sequence));
  if (length <= 0 || static_cast<size_t>(length) >= capacity) return 0U;
  return static_cast<size_t>(length);
}

inline bool parseAck(char* line, uint16_t& sequence) {
  if (line == nullptr) return false;
  char* separator = strchr(line, '|');
  if (separator == nullptr) return false;
  *separator = '\0';
  if (strcmp(line, "ACK") != 0 || strchr(separator + 1, '|') != nullptr) {
    return false;
  }
  unsigned long parsed = 0;
  if (!parseUnsigned(separator + 1, UINT16_MAX, parsed) || parsed == 0U) {
    return false;
  }
  sequence = static_cast<uint16_t>(parsed);
  return true;
}

inline bool isHello(const char* line) {
  return line != nullptr && strcmp(line, "HELLO|1") == 0;
}

}  // namespace robot_uart
