#!/usr/bin/env python3
"""
=====================================================================
 Smart Museum — QR code generator for the gate
=====================================================================
Generates the QR images the visitors scan. Run it whenever you change
the WiFi name/password or the Hub IP, then print the PNGs.

  Install once :  pip install "qrcode[pil]"
  Run          :  python tools/generate_qr.py
  Output       :  tools/qr_output/*.png

It prints THREE codes:
  01_wifi_join.png        join the Hub WiFi (native camera)
  02_checkin_inpage.png   scan FROM the gate page's "Scan QR" button
                          (offline jsQR). Encodes the token text below.
  03_checkin_native.png   scan with the phone's normal camera for an
                          auto check-in (no jsQR). Encodes the /api/enter URL.

NOTE: SSID / PASSWORD / HUB_IP must match firmware/hub/include/config.h
=====================================================================
"""
import os
import qrcode

# ---- keep these in sync with firmware/hub/include/config.h ----
SSID          = "MuseumHub"
PASSWORD      = "museum2026"
HUB_IP        = "192.168.4.1"
CHECKIN_TOKEN = "MUSEUM-ENTRY-2026"   # text the in-page scanner reads

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "qr_output")
os.makedirs(OUT, exist_ok=True)


def make(filename: str, data: str) -> None:
    img = qrcode.make(data)
    img.save(os.path.join(OUT, filename))
    print(f"  {filename:24s}  <-  {data}")


def main() -> None:
    print(f"Generating QR codes into: {OUT}\n")
    make("01_wifi_join.png",        f"WIFI:T:WPA;S:{SSID};P:{PASSWORD};H:false;;")
    make("02_checkin_inpage.png",   CHECKIN_TOKEN)
    make("03_checkin_native.png",   f"http://{HUB_IP}/api/enter")
    print("\nDone. Print these and post them at the gate.")
    print("02 = in-page scanner (needs jsQR in data/js/vendor/).")
    print("03 = simplest (any phone's native camera). Both reach the same approval flow.")


if __name__ == "__main__":
    main()
