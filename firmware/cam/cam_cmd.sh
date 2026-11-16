#!/usr/bin/env bash
# cam_cmd.sh -- send one command to the ESP32-CAM over USB serial and print its reply.
#
# The CAM-MB has NO auto-reset, so opening this port does NOT reboot the cam --
# the firmware keeps running and answers immediately. If you get NO reply, the
# cam is probably back in download mode: press its RST button once (BOOT *not*
# held) and retry.
#
# Capture commands (what you'll use most):
#   ./cam_cmd.sh "SHOOT 1"     # capture + join MuseumHub + push JPEG to the Hub (station 1)
#   ./cam_cmd.sh "SHOOT 3"     # ...tag it as station 3 instead
#   ./cam_cmd.sh "CAPTURE 2"   # capture to RAM only (uploads only if WiFi already up)
#   ./cam_cmd.sh "SEND 2"      # re-upload the last captured JPEG
#   ./cam_cmd.sh "AUTO 5000"   # auto capture+upload every 5 s
#   ./cam_cmd.sh "AUTO"        # stop auto-capture
#
# Diagnostics:
#   ./cam_cmd.sh "STATUS"      # camera / WiFi / RAM / SD diagnostics
#   ./cam_cmd.sh "LIST"        # list photos saved on the cam's (optional) SD
#   ./cam_cmd.sh "INIT"        # re-init camera + try mounting SD
#   ./cam_cmd.sh "FLASH ON"    # / "FLASH OFF"  -- onboard white LED
#   ./cam_cmd.sh "HELP"
#
# Options:
#   CAM_PORT=/dev/ttyUSB1 ./cam_cmd.sh "SHOOT 1"     # override port
#   ./cam_cmd.sh "SHOOT 1" 20                        # 2nd arg = seconds to listen (default 12)
#
# If the Arduino IDE Serial Monitor is holding the port, close it first (or run:
# pkill -x serial-monitor). Only coreutils needed (stty/printf/cat/timeout).

set -euo pipefail
PORT="${CAM_PORT:-/dev/ttyUSB1}"
CMD="${1:-STATUS}"
WAIT="${2:-12}"

if [[ ! -e "$PORT" ]]; then
  echo "cam_cmd: port $PORT not found. Set CAM_PORT=...  (try: ls /dev/ttyUSB*)" >&2
  exit 1
fi

stty -F "$PORT" 115200 raw -echo -echoe -echok -echoctl -echoke 2>/dev/null || true

echo "cam_cmd: -> $CMD   (port $PORT, listening ${WAIT}s)"
printf '%s\n' "$CMD" >"$PORT"               # send the command
timeout "$WAIT" cat "$PORT" || true         # print the reply
echo
echo "cam_cmd: done. (no output? press the cam's RST button once and retry)"
