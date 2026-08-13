#pragma once
// =====================================================================
//  ble_ota.h  —  Over-the-air firmware update over Bluetooth LE (NimBLE).
//
//  WHY BLE INSTEAD OF Wi-Fi (espota) HERE
//    The Hub already runs the "MuseumHub" SoftAP + ESP-NOW on the 2.4 GHz
//    Wi-Fi radio. Wi-Fi OTA (espota) would ride that same AP and force the
//    flashing laptop to leave its internet Wi-Fi to join MuseumHub. BLE is a
//    SEPARATE radio, so the laptop keeps its internet connection and pushes
//    the image over Bluetooth -- no cable, no Wi-Fi switch.
//
//  HOW IT WORKS
//    The ESP advertises a custom GATT service (see ble_ota.cpp for the UUIDs).
//    tools/ble_ota_upload.py (bleak) writes the image in chunks; the ESP
//    streams them into the INACTIVE OTA partition with esp_ota_* and reboots
//    into it. The dual-slot partitions.csv is still required (esp_ota writes
//    the inactive slot).
//
//  THREADING
//    NimBLE runs its own host task; callbacks fire asynchronously, so -- unlike
//    ArduinoOTA -- there is no handle() to pump. bleOtaInProgress() goes true
//    during a flash so loop() on Core 1 parks the LED/siren/ESP-NOW show
//    (nothing else must touch flash mid-write).
// =====================================================================

// Initialise NimBLE, create the OTA GATT service, and start advertising.
// Call once, after WiFi SoftAP + ESP-NOW are already up.
void bleOtaBegin();

// True while a BLE firmware flash is in progress (Core 1 loop parks itself).
bool bleOtaInProgress();
