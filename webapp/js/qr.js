// =====================================================================
//  qr.js  —  read a QR code from a photo the visitor takes with the camera.
//
//  Why a photo and not live camera streaming?
//    Browsers only allow live camera (getUserMedia) on a "secure context"
//    (https or localhost). The ESP32 Hub serves plain HTTP at 192.168.4.1,
//    so live streaming would be blocked. A file/camera <input> works over
//    plain HTTP — on a phone it still opens the camera to snap the QR.
//
//  Offline requirement:
//    Download jsQR (a small free library) once and put it at:
//        webapp/js/vendor/jsQR.js
//    It is loaded by index.html. Without it, visitors use "Simulate".
//    Get it from: https://github.com/cozmo/jsQR  (use jsQR.js, the plain file)
// =====================================================================

const Qr = (() => {
  // Returns true if the offline decoder is available.
  function available() {
    return typeof window.jsQR === "function";
  }

  // Decode a single image File. Calls onResult(text) if a QR is found,
  // otherwise onResult(null).
  function decodeFile(file, onResult) {
    if (!available()) { onResult(null); return; }
    const reader = new FileReader();
    reader.onload = (e) => {
      const img = new Image();
      img.onload = () => {
        const canvas = document.createElement("canvas");
        canvas.width = img.width;
        canvas.height = img.height;
        const ctx = canvas.getContext("2d", { willReadFrequently: true });
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
        const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        const code = window.jsQR(imageData.data, imageData.width, imageData.height);
        onResult(code ? code.data : null);
      };
      img.src = e.target.result;
    };
    reader.readAsDataURL(file);
  }

  // Fake a scan for demos / when no camera or library is present.
  function simulate() {
    return "MUSEUM-VISITOR-" + Math.floor(1000 + Math.random() * 9000);
  }

  return { available, decodeFile, simulate };
})();
