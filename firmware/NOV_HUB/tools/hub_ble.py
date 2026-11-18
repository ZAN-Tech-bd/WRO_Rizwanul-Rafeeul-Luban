#!/usr/bin/env python3
"""Monitor the Museum Hub or robot and install a signed application over Bluetooth LE."""

import argparse
import asyncio
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import time

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

SERVICE, COMMAND, DATA, STATUS, CHALLENGE, DIAGNOSTICS = (
    f"8e7f1a{number:02x}-9b3c-4e2a-8d11-7c0e9f3a2b10" for number in range(1, 7)
)
BEGIN, FINISH = 1, 2
WRITING, DONE = 2, 3
ERRORS = ["none", "invalid command", "signature rejected", "invalid size",
          "flash or image validation failed", "digest mismatch", "transfer timed out"]
DEFAULT_KEY = Path(__file__).resolve().parents[1] / ".ota-signing.key"


async def find_hub(address, timeout=8):
    devices = await BleakScanner.discover(timeout, return_adv=True, service_uuids=[SERVICE])
    matches = [device for device, adv in devices.values()
               if SERVICE in adv.service_uuids and (address is None or device.address.lower() == address.lower())]
    if len(matches) > 1:
        raise ValueError("Multiple Hubs found. Use --address with one of " + ", ".join(d.address for d in matches))
    if not matches:
        raise BleakError(f"NOV_HUB BLE service not found{f' at {address}' if address else ''}.")
    return matches[0]


async def read(client, characteristic):
    return await asyncio.wait_for(client.read_gatt_char(characteristic), timeout=10)


async def write(client, characteristic, payload):
    await asyncio.wait_for(client.write_gatt_char(characteristic, payload, response=True), timeout=10)


async def read_diagnostics(client):
    line = (await read(client, DIAGNOSTICS)).decode("ascii")
    if not line.startswith(("NOV_HUB ", "NOV_ROBOT ")):
        raise ValueError(f"Unexpected diagnostics value {line!r}")
    return line


def fields(line):
    return dict(value.split("=", 1) for value in line.split()[1:] if "=" in value)


async def monitor(device, once):
    async with BleakClient(device, timeout=15) as client:
        while True:
            print(await read_diagnostics(client), flush=True)
            if once:
                return
            await asyncio.sleep(1)


async def expect_status(client, expected, size):
    state, written, error = struct.unpack(">BIB", await read(client, STATUS))
    if state != expected or written != size or error:
        reason = ERRORS[error] if error < len(ERRORS) else f"error {error}"
        raise ValueError(f"Hub status {state}, {written} bytes ({reason}); expected {expected}, {size} bytes.")


def fingerprint(image):
    # The ESP image's first segment starts at offset 32 with esp_app_desc_t.
    if len(image) < 180 or image[0] != 0xE9 or image[32:36] != bytes.fromhex("3254cdab"):
        raise ValueError("Expected a PlatformIO firmware.bin application image.")
    if struct.unpack_from("<H", image, 12)[0] != 9:
        raise ValueError("Expected an ESP32-S3 application image.")
    if DIAGNOSTICS.encode("ascii") not in image:
        raise ValueError("The application is missing the Museum BLE diagnostics service.")
    return image[176:180].hex()  # First four bytes of esp_app_desc_t.app_elf_sha256.


async def transfer(client, image, key):
    manifest = struct.pack(">BI", BEGIN, len(image)) + hashlib.sha256(image).digest()
    nonce = await read(client, CHALLENGE)
    if len(nonce) != 16:
        raise ValueError("Unexpected OTA challenge length.")
    signature = subprocess.run(
        ["openssl", "dgst", "-sha256", "-sign", str(key)],
        input=nonce + manifest, stdout=subprocess.PIPE, check=True,
    ).stdout

    # BlueZ's Bleak backend needs an explicit MTU query to discover values above 23.
    acquire_mtu = getattr(getattr(client, "_backend", None), "_acquire_mtu", None)
    if acquire_mtu:
        await asyncio.wait_for(acquire_mtu(), timeout=10)
    mtu = client.mtu_size
    chunk = min(512, mtu - 3)
    print(f"Sending {len(image):,} bytes in {chunk}-byte chunks (MTU {mtu}).", flush=True)
    await write(client, COMMAND, manifest + signature)
    await expect_status(client, WRITING, 0)

    next_report = 5
    for offset in range(0, len(image), chunk):
        block = image[offset:offset + chunk]
        sent = offset + len(block)
        try:
            await write(client, DATA, block)
        except BleakError:
            if not client.is_connected:
                raise
            # The ATT acknowledgment can fail after the ESP has written the
            # block. Query the flash offset before deciding to send it again.
            state, written, error = struct.unpack(">BIB", await read(client, STATUS))
            if state != WRITING or error or written not in (offset, sent):
                raise
            if written == offset:
                await write(client, DATA, block)
        if sent * 100 // len(image) >= next_report:
            await expect_status(client, WRITING, sent)
            print(f"{next_report:3}%  {sent:,}/{len(image):,} bytes", flush=True)
            next_report += 5

    try:
        await write(client, COMMAND, bytes([FINISH]))
        await expect_status(client, DONE, len(image))
        print("Hub verified and committed the image. Waiting for reboot.", flush=True)
    except (BleakError, TimeoutError):
        print("Lost the link during commit. Checking the running build after reconnect.", flush=True)


async def flash(device, image, key, expected_build):
    async with BleakClient(device, timeout=15) as client:
        line = await read_diagnostics(client)
        equipment = line.split()[0]
        if (equipment + " boot=").encode("ascii") not in image:
            raise ValueError(f"The image does not match the connected {equipment}.")
        previous = fields(line)
        if equipment == "NOV_ROBOT" and previous.get("state") != "idle":
            raise ValueError("Stop the robot before updating.")
        if previous.get("emergency") != "off":
            raise ValueError("Clear the Hub emergency before updating.")
        await transfer(client, image, key)

    deadline = time.monotonic() + 45
    latest = None
    while time.monotonic() < deadline:
        try:
            device = await find_hub(device.address, timeout=3)
            async with BleakClient(device, timeout=8) as client:
                line = await read_diagnostics(client)
                latest = fields(line)
                if (latest.get("boot") != previous.get("boot") and latest.get("slot") != previous.get("slot")
                        and latest.get("build") == expected_build):
                    print("OTA boot verified.")
                    print(line)
                    return
        except (BleakError, TimeoutError):
            continue
    if latest and latest.get("emergency") == "on":
        raise ValueError("Reboot is waiting for the Hub emergency to clear.")
    raise ValueError("Could not verify a reboot into the expected application slot and build.")


async def run(args):
    if args.mode == "flash":
        image = args.firmware.read_bytes()
        expected_build = fingerprint(image)
        if not args.key.is_file():
            raise ValueError(f"OTA signing key missing at {args.key}.")
        public_key = subprocess.check_output(["openssl", "pkey", "-in", str(args.key), "-pubout"])
        if public_key not in image:
            raise ValueError("The application was built with a different OTA key. Rebuild using the Hub's current key.")
    device = await find_hub(args.address)
    print(f"Found {device.name or 'NOV_HUB'} at {device.address}", flush=True)
    if args.mode == "monitor":
        await monitor(device, args.once)
    else:
        await flash(device, image, args.key, expected_build)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--address", help="BLE device address, if multiple Hubs are nearby")
    modes = parser.add_subparsers(dest="mode", required=True)
    monitor_parser = modes.add_parser("monitor", help="stream the Hub status line to the terminal")
    monitor_parser.add_argument("--once", action="store_true", help="read a single status snapshot")
    flash_parser = modes.add_parser("flash", help="sign, upload and verify a firmware.bin")
    flash_parser.add_argument("firmware", type=Path)
    flash_parser.add_argument("--key", type=Path, default=DEFAULT_KEY)
    try:
        asyncio.run(run(parser.parse_args()))
    except KeyboardInterrupt:
        return 130
    except (BleakError, OSError, ValueError, TimeoutError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
