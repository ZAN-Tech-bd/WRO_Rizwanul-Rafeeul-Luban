// =====================================================================
//  qr.js  —  read a QR code from a photo the visitor takes with the camera.
//
//  Why a photo and not live camera streaming?
//    Browsers only allow live camera (getUserMedia) on a "secure context"
//    (https or localhost). The ESP32 Hub serves plain HTTP at 192.168.4.1,
//    so live streaming would be blocked. A file/camera <input> works over
//    plain HTTP — on a phone it still opens the camera to snap the QR.
//
//  Offline decoder:
//    jsQR 1.4.0 is vendored at webapp/js/vendor/jsQR.js and loaded by
//    index.html, so image decoding works without internet access.
// =====================================================================

const Qr = (() => {
  // Returns true if the offline decoder is available.
  function available() {
    return typeof window.jsQR === "function";
  }

  // Decode one camera/gallery image. Calls onResult(text, null) if a QR is
  // found, otherwise onResult(null, error). Large phone photos are scaled down
  // first so decoding stays responsive and does not allocate a huge canvas.
  function decodeFile(file, onResult) {
    if (!available()) {
      onResult(null, new Error("QR decoder is unavailable"));
      return;
    }
    if (!file) {
      onResult(null, new Error("No image was selected"));
      return;
    }

    const objectUrl = URL.createObjectURL(file);
    const img = new Image();
    let finished = false;

    const finish = (text, error = null) => {
      if (finished) return;
      finished = true;
      URL.revokeObjectURL(objectUrl);
      onResult(text, error);
    };

    img.onload = () => {
      try {
        const sourceWidth = img.naturalWidth || img.width;
        const sourceHeight = img.naturalHeight || img.height;
        if (!sourceWidth || !sourceHeight) throw new Error("Image has no dimensions");

        const maxDimension = 1600;
        const scale = Math.min(1, maxDimension / Math.max(sourceWidth, sourceHeight));
        const canvas = document.createElement("canvas");
        canvas.width = Math.max(1, Math.round(sourceWidth * scale));
        canvas.height = Math.max(1, Math.round(sourceHeight * scale));
        const ctx = canvas.getContext("2d", { willReadFrequently: true });
        if (!ctx) throw new Error("Canvas is unavailable");
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
        const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        const code = window.jsQR(imageData.data, imageData.width, imageData.height, {
          inversionAttempts: "attemptBoth"
        });
        finish(code ? code.data : null, code ? null : new Error("No QR code found"));
      } catch (error) {
        finish(null, error);
      }
    };
    img.onerror = () => finish(null, new Error("The selected image could not be opened"));
    img.src = objectUrl;
  }

  // Fake a scan for demos / when no camera or library is present.
  function simulate() {
    return "MUSEUM-VISITOR-" + Math.floor(1000 + Math.random() * 9000);
  }

  return { available, decodeFile, simulate };
})();
