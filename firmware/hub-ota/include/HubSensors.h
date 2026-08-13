#pragma once
// =====================================================================
//  HubSensors.h  —  DHT11 + MQ pin scan for sensors wired to the Hub itself.
//
//  The loom was built before anyone wrote down which sensor went to which
//  GPIO, so this probes EVERY pin in HUB_SENSOR_PINS as a DHT11 in turn and
//  reports, per pin, exactly what came back. Pins that carry an ADC1 channel
//  are also sampled as an analog (MQ-style) input on the same pass, because
//  that is the only thing an input-only pin can actually do.
//
//  Deliberately hand-rolled instead of pulling in Adafruit's DHT library: the
//  point here is diagnosis, and a library that returns NaN for every failure
//  cannot tell "nothing is wired here" apart from "a DHT11 is wired here but
//  the frame was corrupt". The five outcomes below can.
// =====================================================================

#include <Arduino.h>
#include "config.h"

// What one DHT11 probe of one pin produced.
enum DhtResult : uint8_t {
  DHT_UNPROBED = 0,   // the scan has not reached this pin yet
  DHT_OK,             // 40 bits with a matching checksum -> a DHT11 lives here
  DHT_LINE_LOW,       // pin already LOW before the start pulse (short, or an
                      // analog output sitting below the logic threshold)
  DHT_NO_RESPONSE,    // the line never answered the 20 ms start pulse
  DHT_FRAME_TIMEOUT,  // it answered, then stalled part-way through the 40 bits
  DHT_CHECKSUM,       // all 40 bits arrived but the checksum did not match
  DHT_NO_DRIVE,       // input-only pin (GPIO34-39): the chip has no output
                      // driver, so the sensor could not be asked in the first
                      // place. Not a wiring fault -- move the DHT11 to a pin
                      // that can drive, and use this one for an MQ analog out.
};

// Is anything electrically attached to a pin at all? Answers the question a
// silent DHT11 probe cannot: "no_response" on a pin with no ADC could mean an
// empty header or a sensor that is wired but not talking, and those need very
// different fixes. Needs internal pull-ups, so GPIO34-39 report ATTACH_UNKNOWN.
enum PinAttach : uint8_t {
  ATTACH_UNKNOWN = 0, // input-only pin: no internal pulls to test with
  ATTACH_FLOATING,    // follows both pulls -> open circuit, nothing connected
  ATTACH_PULLED_HIGH, // held high against the pull-down -> a pull-up resistor,
                      // which is what an idle DHT11/module data line looks like
  ATTACH_PULLED_LOW,  // held low against the pull-up -> tied to GND or a driver
  ATTACH_LOADED,      // inverted result: a capacitive or otherwise odd load
};

// Short machine-readable names for the JSON + logs.
const char* dhtResultName(uint8_t result);
const char* pinAttachName(uint8_t attach);

// Give every artifact a row: measured where a sensor reports, interpolated
// from the real boards elsewhere. Call from loop().
void healthFillStations();

void sensorsBegin();    // call once from setup()
void sensorsUpdate();   // call from loop(); probes one pin per HUB_SENSOR_PERIOD_MS
