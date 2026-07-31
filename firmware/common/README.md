# common/ — shared ESP-NOW packet

`MuseumProtocol.h` defines the **one** ESP-NOW message every node uses
(`museum::MuseumMessage`, 24 bytes: magic, version, type, nodeId, stationId,
flags, temp×10, humidity×10, gas, count, sequence, uptime, checksum).

Every project includes it via `build_flags = -I ../common` in its `platformio.ini`.
**Keep this file identical across all nodes** — change it in one place only.
