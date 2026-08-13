#pragma once

// The monitor page markup, kept out of the .ino deliberately.
//
// Both the Arduino and PlatformIO builders run a prototype generator over .ino
// files. It scans the raw text without understanding string literals, so the
// JavaScript "function updateLog()" and "function clearLog()" below get read as
// C++ declarations and the build dies with:
//
//     error: 'function' does not name a type; did you mean 'union'?
//
// Headers are passed through untouched, so the page lives here instead. Held in
// PROGMEM and served with send_P, which also keeps a ~3 KB copy off the heap on
// every request.

#include <Arduino.h>

static const char rfidPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>ESP32 RFID Monitor</title>

<style>

body {
  font-family: Arial, sans-serif;
  background: #111;
  color: #eee;
  margin: 0;
  padding: 20px;
}

h1 {
  color: #00e5ff;
}

.status {
  padding: 10px;
  background: #222;
  border-radius: 8px;
  margin-bottom: 15px;
}

.log {
  background: #000;
  color: #00ff66;
  padding: 15px;
  border-radius: 8px;
  white-space: pre-wrap;
  font-family: monospace;
  min-height: 300px;
  overflow-y: auto;
}

button {
  padding: 10px 15px;
  margin-top: 10px;
  border: none;
  border-radius: 5px;
  background: #00e5ff;
  color: #000;
  font-weight: bold;
}

</style>

<script>

function updateLog() {

  fetch('/log')
    .then(response => response.text())
    .then(data => {

      document.getElementById('log').innerText = data;

      // Automatically scroll to bottom
      const logBox = document.getElementById('log');
      logBox.scrollTop = logBox.scrollHeight;

    });

}

function clearLog() {

  fetch('/clear')
    .then(() => updateLog());

}

setInterval(updateLog, 500);

</script>

</head>

<body>

<h1>ESP32 RFID Monitor</h1>

<div class="status">
  <b>WiFi:</b> SoftAP<br>
  <b>IP:</b> 192.168.4.1<br>
  <b>RFID SS:</b> GPIO 5
</div>

<div id="log" class="log">
Loading...
</div>

<button onclick="clearLog()">Clear Log</button>

</body>
</html>
)rawliteral";
