Put the web app files here, then upload this folder to the ESP32 as LittleFS.

How:
  1. Copy everything from  ../../../../webapp/  into this  data/  folder,
     keeping the subfolders. You should end up with:

         data/index.html
         data/visitor.html
         data/admin.html
         data/css/style.css
         data/js/data.js
         data/js/hub.js
         data/js/qr.js
         data/js/vendor/jsQR.js        <-- download jsQR.js for offline QR

  2. In Arduino IDE, install "LittleFS_esp32" if prompted, then use:
         Tools -> ESP32 Sketch Data Upload
     (or the "arduino-littlefs-upload" VS Code extension).
     This flashes data/ to the ESP32 flash filesystem.

  3. Re-upload the sketch (track_hub.ino) if not already on the board.

The Hub serves these files from LittleFS at http://192.168.4.1/ .
