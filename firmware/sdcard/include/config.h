// =====================================================================
//  config.h — SD card pin definitions for the ESP32.
//
//  These are the DEFAULT VSPI pins on a standard ESP32 dev board, so no
//  re-mapping is strictly required. They are defined here so the wiring is
//  explicit and easy to change if you ever move to custom HSPI/soft-SPI.
//
//      SD module    ESP32 GPIO
//      ----------   ----------
//      VCC          5V   *  (module's onboard AMS1117-3.3 drops it for the card)
//      GND          GND
//      MISO         19   (VSPI Q / MISO)
//      MOSI         23   (VSPI D / MOSI)
//      SCK          18   (VSPI CLK)
//      CS / SS      5    (VSPI CS)
//
//  * VOLTAGE NOTE: The SD card itself is a 3.3V device. The common red/blue
//    microSD modules with an AMS1117-3.3 regulator + 74LVC125A level shifter
//    accept a 5V VCC safely and present 3.3V logic on MISO back to the ESP32.
//    If your module is a BARE breakout with NO level shifter (just the SD
//    socket + pullups), power it from 3.3V instead — NEVER feed 5V into the
//    ESP32 GPIO pins.
// =====================================================================
#pragma once

#ifndef SD_SCK_PIN
  #define SD_SCK_PIN   18   // VSPI SCK
#endif
#ifndef SD_MISO_PIN
  #define SD_MISO_PIN  19   // VSPI MISO
#endif
#ifndef SD_MOSI_PIN
  #define SD_MOSI_PIN  23   // VSPI MOSI
#endif
#ifndef SD_CS_PIN
  #define SD_CS_PIN    5    // VSPI CS / SS
#endif

// SPI bus speed. 4 MHz is the rock-solid default; bump to 20-40 MHz once the
// card is proven healthy on your wiring. Too fast on long jumper wires = CRC errors.
#ifndef SD_SPI_FREQ_MHZ
  #define SD_SPI_FREQ_MHZ 4
#endif
