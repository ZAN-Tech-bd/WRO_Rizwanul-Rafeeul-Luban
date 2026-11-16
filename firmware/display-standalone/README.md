# Smooth Smiling Robot Face

A calm animated face for this exact display setup:

- Arduino Mega 2560
- HVGA 480x320 3.2-inch TFT LCD shield
- ILI9481 controller
- 16-bit parallel bus

The eyes blink and look left/right smoothly. The sketch redraws only the pixels
that changed, which avoids the visible tearing caused by repainting both eyes on
every frame.

## What you need

- Arduino Mega 2560
- The ILI9481 16-bit Mega shield described above
- A USB data cable
- Arduino IDE 2.x

No library installation is required. The display driver and drawing code are
included in the sketch.

## Upload with Arduino IDE

1. Download this repository with **Code > Download ZIP**, then extract it.
2. Open `SmilingRobotFace/SmilingRobotFace.ino` in Arduino IDE.
3. Select **Tools > Board > Arduino AVR Boards > Arduino Mega or Mega 2560**.
4. Select **Tools > Processor > ATmega2560 (Mega 2560)** if that option appears.
5. Select the Mega's USB port under **Tools > Port**.
6. Click **Upload**.

The display starts the smiling face automatically. There are no menus, serial
commands, or language labels.

## Shield pin map

The shield plugs directly onto the Mega. You do not normally wire these pins by
hand.

| TFT signal | Mega 2560 pins |
|---|---|
| Data D15..D8 | 29..22 |
| Data D7..D0 | 30..37 |
| RD | 43 |
| WR | 39 |
| RS / CD | 38 |
| CS | 40 |
| RESET | 41 |

## Troubleshooting

### Arduino IDE cannot find the port

- Use a USB **data** cable, not a charge-only cable.
- Reconnect the Mega and reopen **Tools > Port**.
- Close Serial Monitor before uploading.

### Upload fails

- Confirm the board is **Arduino Mega or Mega 2560**, not Arduino Uno.
- Disconnect anything else attached to pins 0 and 1 while uploading.
- Press the Mega reset button once, then upload again.

### Screen stays white or blank

- Press the shield firmly and evenly onto every Mega header.
- Confirm the rear label says **ILI9481**, **480x320**, **3.2 inch**, and
  **16bit bus for Arduino Mega 2560**.
- This sketch is for that tested shield. Other controllers and 8-bit shields
  use different initialization or wiring.
- Power the Mega from a reliable USB port or 5 V supply.

### Face is upside down

Change this line in `MegaIli9481::begin()`:

```cpp
static const uint8_t landscape[] = {0x28};
```

to:

```cpp
static const uint8_t landscape[] = {0xE8};
```

Then upload again.

## Animation notes

- Update cap: 30 frames per second
- Blink: eased close, short hold, softer reopening
- Gaze: deterministic left/right movement with smoothstep easing
- During a blink, gaze is frozen so the pupils do not jump
- Eye housings are drawn once; only pupil footprints and eyelid scanlines are
  updated during animation

## Credits and license

The Mega 16-bit pin mapping and ILI9481 initialization are adapted from
[MCUFRIEND_kbv](https://github.com/prenticedavid/MCUFRIEND_kbv) by David
Prentice. See `THIRD_PARTY_LICENSES.md` for its FreeBSD license.

The original code in this repository is released under the MIT License. See
`LICENSE`.
