#include <WiFi.h>
#include <WebServer.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

#include "RfidPage.h"

// =====================================================
// WiFi SoftAP
// =====================================================

const char* AP_SSID = "ESP32-RFID";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

// =====================================================
// RFID
// =====================================================

MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// =====================================================
// Web log
// =====================================================

// Stores recent Serial Monitor messages
String webLog = "";

void addLog(String message) {
  Serial.println(message);

  webLog += message + "\n";

  // Prevent the log from becoming too large
  if (webLog.length() > 8000) {
    webLog = webLog.substring(webLog.length() - 6000);
  }
}

// =====================================================
// Web page
// =====================================================

void handleRoot() {
  // Markup lives in RfidPage.h -- see the note there for why.
  server.send_P(200, "text/html", rfidPage);
}

// =====================================================
// Send log to browser
// =====================================================

void handleLog() {
  server.send(200, "text/plain", webLog);
}

// =====================================================
// Clear web log
// =====================================================

void handleClear() {
  webLog = "";
  server.send(200, "text/plain", "OK");
}

// =====================================================
// Setup
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(500);

  // ---------------------------------------------------
  // Start SoftAP
  // ---------------------------------------------------

  WiFi.mode(WIFI_AP);

  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress IP = WiFi.softAPIP();

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32 RFID + SoftAP");
  Serial.println("==============================");

  Serial.print("WiFi SSID: ");
  Serial.println(AP_SSID);

  Serial.print("WiFi Password: ");
  Serial.println(AP_PASSWORD);

  Serial.print("Web Server IP: ");
  Serial.println(IP);

  // ---------------------------------------------------
  // Web server routes
  // ---------------------------------------------------

  server.on("/", handleRoot);
  server.on("/log", handleLog);
  server.on("/clear", handleClear);

  server.begin();

  addLog("Web server started");
  addLog("Connect to WiFi: " + String(AP_SSID));
  addLog("Open browser: http://" + IP.toString());

  // ---------------------------------------------------
  // RFID initialization
  // ---------------------------------------------------

  addLog("Initializing MFRC522...");

  mfrc522.PCD_Init();

  addLog("MFRC522 initialized");

  // Show MFRC522 information on Serial
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);

  addLog("Scan PICC to see UID");
}

// =====================================================
// Main loop
// =====================================================

void loop() {

  // Handle web requests
  server.handleClient();

  // ---------------------------------------------------
  // Check for new RFID card
  // ---------------------------------------------------

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // ---------------------------------------------------
  // Print UID
  // ---------------------------------------------------

  Serial.println();
  Serial.println("==============================");

  addLog("RFID CARD DETECTED");

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, mfrc522.uid);
  Serial.println();

  // ---------------------------------------------------
  // Convert UID to String
  // ---------------------------------------------------

  String uidString = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {

    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0";
    }

    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }

  uidString.toUpperCase();

  addLog("UID: " + uidString);

  Serial.println("==============================");

  // Halt card
  mfrc522.PICC_HaltA();
}
