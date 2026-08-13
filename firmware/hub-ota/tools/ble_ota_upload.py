#!/usr/bin/env python3
"""
BLE OTA uploader for the Museum Hub (NimBLE).

Pushes a firmware.bin to the Hub over Bluetooth LE using the custom GATT OTA
service defined in firmware/hub-ota/src/ble_ota.cpp. The Hub keeps running its
MuseumHub SoftAP + ESP-NOW on Wi-Fi the whole time, and BLE is a separate radio,
so the flashing laptop does NOT have to join MuseumHub or give up its internet.

Install once:
    pip install bleak

Flash (no cable, after the first USB install of the BLE-OTA firmware):
    cd firmware/hub-ota
    pio run -e hub                       # build -> .pio/build/hub/firmware.bin
    python3 tools/ble_ota_upload.py .pio/build/hub/firmware.bin

The password must match BLE_OTA_PASSWORD in include/config.h. Pass it with
--password, or set the BLE_OTA_PASSWORD environment variable to avoid it
showing up in shell history:
    BLE_OTA_PASSWORD='...' python3 tools/ble_ota_upload.py firmware.bin

The script scans for the Hub by its OTA service UUID (falls back to the
"MuseumHub-BLE-OTA" name), sends AUTH(password) then BEGIN(size), streams DATA
chunks (write-with-response, so every chunk is acked), verifies the byte count
via STATUS, then sends FINISH -- the Hub commits the inactive OTA slot and
reboots into it. The Hub disconnects after 5 wrong AUTH attempts on one
connection, so a mistyped password just means reconnect and retry.
"""

import argparse
import asyncio
import getpass
import os
import struct
import sys
import time

from bleak import BleakClient, BleakScanner

SERVICE_UUID = "8e7f1a01-9b3c-4e2a-8d11-7c0e9f3a2b10"
CMD_UUID     = "8e7f1a02-9b3c-4e2a-8d11-7c0e9f3a2b10"
DATA_UUID    = "8e7f1a03-9b3c-4e2a-8d11-7c0e9f3a2b10"
STATUS_UUID  = "8e7f1a04-9b3c-4e2a-8d11-7c0e9f3a2b10"

OP_AUTH, OP_BEGIN, OP_FINISH = 0x00, 0x01, 0x02

ST_AUTH_OK = 0x01
ST_ERROR   = 0xFF


async def find_device(timeout=15.0):
    print(f"[scan] looking for Hub OTA service ({SERVICE_UUID})...")
    # return_adv=True: BLEDevice.metadata was removed from recent bleak
    # releases, so advertised service UUIDs now only come via the paired
    # AdvertisementData returned alongside each device.
    found = await BleakScanner.discover(timeout, return_adv=True, service_uuids=[SERVICE_UUID])
    # Prefer a match on the advertised service UUID; fall back to the name.
    for device, adv in found.values():
        uuids = adv.service_uuids or []
        if any(SERVICE_UUID.lower() == str(u).lower() for u in uuids):
            return device
    for device, adv in found.values():
        if device.name and "MuseumHub" in device.name:
            return device
    raise SystemExit("Hub not found. Is it powered on and advertising BLE OTA?")


async def main(path, password):
    data = open(path, "rb").read()
    print(f"[file ] {path}  {len(data)} bytes")

    dev = await find_device()
    print(f"[scan ] found {dev.name!r} ({dev.address})")

    async with BleakClient(dev.address, timeout=30.0) as client:
        # On BlueZ, mtu_size reports the 23-byte default until the real value
        # is acquired, which would drop us to 20-byte chunks and turn a ~4
        # minute upload into well over an hour. Ask for the true MTU first.
        mtu = None
        try:
            if hasattr(client, "_acquire_mtu"):
                await client._acquire_mtu()
            mtu = getattr(client, "mtu_size", None)
        except Exception:
            mtu = None
        # ATT write payload max = MTU - 3 (header). A reported 23 means "not
        # negotiated yet", not a genuinely tiny link, so prefer the proven
        # 200-byte fallback over crawling.
        chunk = max(20, mtu - 3) if mtu and mtu > 23 else 200
        print(f"[mtu  ] negotiated={mtu}  chunk={chunk}")

        # 1) AUTH: opcode + password bytes. Must match BLE_OTA_PASSWORD in
        #    include/config.h, or the Hub refuses BEGIN (and disconnects
        #    after 5 wrong attempts on one connection).
        await client.write_gatt_char(
            CMD_UUID, bytes([OP_AUTH]) + password.encode("utf-8"), response=True
        )
        st = await client.read_gatt_char(STATUS_UUID)
        if st[0] != ST_AUTH_OK:
            raise SystemExit(
                "AUTH rejected by the Hub -- wrong password? "
                "Check --password / BLE_OTA_PASSWORD against include/config.h."
            )
        print("[auth ] ok")

        # 2) BEGIN: opcode + 4-byte big-endian total size.
        await client.write_gatt_char(
            CMD_UUID, bytes([OP_BEGIN]) + struct.pack(">I", len(data)), response=True
        )
        await asyncio.sleep(0.3)

        # 3) Stream DATA (write-with-response -> reliable, flow-controlled).
        sent = 0
        t0 = time.time()
        last_pct = -1
        while sent < len(data):
            block = data[sent:sent + chunk]
            await client.write_gatt_char(DATA_UUID, block, response=True)
            sent += len(block)
            pct = sent * 100 // len(data)
            if pct != last_pct and pct % 5 == 0:
                last_pct = pct
                print(f"[data ] {pct:3d}%  ({sent}/{len(data)})", flush=True)
        dt = time.time() - t0
        print(f"[data ] 100%  ({sent}/{len(data)})  in {dt:.1f}s "
              f"({len(data) / dt / 1024:.1f} KB/s)")

        # 4) Verify via STATUS that the device received exactly the right bytes.
        st = await client.read_gatt_char(STATUS_UUID)
        state = st[0]
        written = struct.unpack(">I", bytes(st[1:5]))[0]
        print(f"[stat ] state={state} written={written}")
        if written != len(data):
            raise SystemExit(f"size mismatch: device has {written}, expected {len(data)}")

        # 5) FINISH -> commit + reboot (the Hub disconnects here).
        # The Hub calls esp_restart() inside the BLE write callback, so it
        # reboots before it can send the write-acknowledgement. The local
        # stack then reports a GATT/disconnect error for a write that in fact
        # succeeded. Since STATUS above already proved every byte landed,
        # treat that specific post-FINISH error as success rather than
        # scaring the operator into re-flashing a Hub that is already updated.
        try:
            await client.write_gatt_char(CMD_UUID, bytes([OP_FINISH]), response=True)
        except Exception as e:
            print(f"[done ] no FINISH ack ({type(e).__name__}) -- expected: "
                  f"the Hub reboots before it can reply.")
        print("[done ] firmware committed -- the Hub is rebooting into the new image.")


def parse_args():
    p = argparse.ArgumentParser(description="Flash the Museum Hub over BLE OTA.")
    p.add_argument("firmware", help="path to firmware.bin")
    p.add_argument(
        "--password",
        default=None,
        help="must match BLE_OTA_PASSWORD in include/config.h "
             "(default: read from BLE_OTA_PASSWORD env var, else prompt)",
    )
    return p.parse_args()


if __name__ == "__main__":
    args = parse_args()
    pw = args.password or os.environ.get("BLE_OTA_PASSWORD")
    if not pw:
        pw = getpass.getpass("BLE OTA password (from include/config.h): ")
    asyncio.run(main(args.firmware, pw))
