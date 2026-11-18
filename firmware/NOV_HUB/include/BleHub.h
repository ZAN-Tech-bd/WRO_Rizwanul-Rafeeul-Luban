#pragma once
#include <stddef.h>

constexpr size_t BLE_HUB_DIAGNOSTICS_CAPACITY = 512;

// BLE application updates and a read-only copy of the USB status line.
bool bleHubBegin(const char* deviceName = "MuseumHub-BLE-OTA", bool (*allowUpdate)() = nullptr);
bool bleHubTransferActive();
void bleHubSetDiagnostics(const char* line);
void bleHubPoll(bool alarmOrButtonActive);
