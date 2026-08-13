# Smart Museum Guidance System — Pseudocode

**Scope locked 2026-08-11.** This describes the system we are actually building, after
the rail pivot. Where it differs from `BUILD_PLAN.md` §2, this file wins.

Dropped for good: line following, IR obstacle stop, language button, queue instruction,
restricted-area gate, admin PIN.

Still to build: **(A)** the robot must tell the Hub where it is, so the station LED can
turn green — and **(B)** the artifact-health node. Both are marked `TO BUILD` below.

---

## The six boards

```
        RFID cards on the rail
                 |
                 v
   [ NANO ] <--UART--> [ ROBOT ESP32 ] --UART--> [ MEGA + TFT ]  face + welcome
   rail motors          brain: cards,   |
                        audio, photos   |  --UART--> [ ESP32-CAM ]  photo
                                        |
                                   ESP-NOW ch1
                                        |
                    [ HUB ESP32 ] <-----+-----> [ HEALTH ESP32 ]  TO BUILD
                    station LEDs, siren,
                    emergency button,
                    Wi-Fi "MuseumHub" + web page
                              |
                        phones / laptop
```

Rules that never change:

1. **Every ESP32 stays on Wi-Fi channel 1.** ESP-NOW dies silently otherwise.
2. **The Hub is the only board that serves web pages to visitors.**
3. **The Hub owns the emergency.** Nobody else decides it.
4. **Nothing waits.** No board ever sleeps or blocks; every timer is "has enough time
   passed since I last did this?"

---

## 1. Robot brain — ESP32 (`ESP32_RFID_MP3_RAIL/`)

The tour leader. Reads the cards, plays the story, starts and stops the rail, asks the
camera for a photo, and obeys the Hub in an emergency.

```
START UP:
    open the USB console (for us to watch)
    start the RFID card reader
    start the MP3 player, set volume to 20
        if it does not answer  ->  remember "no audio", carry on
    start the link to the Nano (9600 baud)
        send "STOP"        (safe: the rail must not move at power-on)
        send "GET"         (ask the Nano what its motor speeds are)
    start the one-way wire to the camera
    turn on our service hotspot "ESP32-MP3-RAIL" on channel 1
    start ESP-NOW on channel 1
        listen for the Hub's emergency message
        register the broadcast address so we can send too      # TO BUILD (A)
    start the little web page (volume + motor balance + event log)
    set  railIsLatchedStopped = TRUE       # nothing moves until the START card

REPEAT FOREVER:

    1. answer any web-page request

    2. IF the Hub's emergency state is different from ours:
           adopt it, and:
           IF emergency just turned ON:
               stop the audio, cancel any waiting photo
               clear the stop latch
               send "START" to the Nano          # drive to the exit
               remember: only the END card matters now
           IF emergency just turned OFF:
               stop the audio
               send "STOP" to the Nano
               set railIsLatchedStopped = TRUE   # a human repositions the robot
                                                 # the START card resumes the tour

    3. IF a card is touching the reader:
           read its UID and halt the card (so one touch = one action)
           IF this is the same card as last time AND less than 1.5 s ago:
               ignore it
           ELSE look the UID up in the card table:

               NOT IN THE TABLE      ->  say "unknown card", do nothing

               EMERGENCY IS ON and this is not the END card:
                                     ->  ignore it, but still report the station
                                         to the Hub (we are passing, not stopping)

               END CARD (A3 C0 38 15):
                                     ->  send "STOP" to the Nano
                                         stop the audio
                                         railIsLatchedStopped = TRUE

               START CARD (18 C4 0F BA):
                                     ->  railIsLatchedStopped = FALSE
                                         play track 1 (welcome: English, then Bangla)
                                         remember: start the rail when it finishes

               STATION CARD (tracks 2..8  ->  stations 1..7):
                                     ->  send "STOP" to the Nano
                                         play that station's track
                                             (one file = English first, then Bangla)
                                         remember: start the rail when it finishes
                                         book a photo for 2 seconds from now
                                         tell the Hub: "at station N, stopped"   # TO BUILD (A)
                                                       -> Hub lights that LED green

    4. IF the MP3 player says "track finished" AND it played for at least 1.5 s:
           IF the stop latch is clear:
               send "START" to the Nano
               tell the Hub: "at station N, moving"      # TO BUILD (A)
           ELSE
               stay stopped

    5. IF the MP3 player reports an error, OR the track has run over 90 seconds:
           treat it as finished, but leave the rail STOPPED  (safety first)

    6. read any reply from the Nano:
           "STATE:RUNNING"   -> the rail is moving
           "STATE:STOPPED"   -> the rail is still
           "TUNE ML=n MR=n"  -> the real motor speeds, show them on the web page

    7. IF a photo is booked AND the audio is still playing AND 2 seconds have passed:
           send "SHOOT <station>" to the camera
           forget the booking (exactly one photo per card)
       IF the audio stopped early, cancel the booking instead

    8. EVERY 1 SECOND:                                        # TO BUILD (A)
           tell the Hub where we are:
               station number, and whether we are moving
           (the Hub forgets us after 2.5 s of silence, so this must keep coming)

    9. handle test keys typed on the USB console (1..9 play a track, +/- volume,
       s stops audio, e fakes an emergency for bench testing)
```

**Message to the Hub (`MSG_LOCATION`)** — one small packet:
`{ station number 1..7, moving? yes/no }`. It is a broadcast: we do not care who hears it.

---

## 2. Rail drive — Arduino Nano (`NANO_LFR_RAIL/`)

A dumb, obedient pair of hands. It has **no sensors**. The rail is curved and the robot
simply drives forward along it, so all it has to do is go or stop.

```
START UP:
    set the six motor pins as outputs
    BRAKE          (both sides shorted: the robot cannot roll)
    load the saved motor speeds from EEPROM (if we ever saved any)
    print "READY:STOPPED"          # tells the ESP32 we are alive and safe

REPEAT FOREVER:
    IF a complete line has arrived from the ESP32:
        make it upper-case, then:

        "START" (or 1 / F / G / W / MAN):
              drive both motors forward:
                  left  channel at ML   (150 on this chassis)
                  right channel at MR   (210 on this chassis)
              reply "STATE:RUNNING"

        "STOP" (or 0 / S / X):
              BRAKE
              reply "STATE:STOPPED"

        "SET ML <0..255>" / "SET MR <0..255>":
              change that speed
              IF we are driving right now, apply it immediately
              reply "TUNE ML=n MR=n"

        "GET":    reply "TUNE ML=n MR=n"
        "SAVE":   write both speeds to EEPROM, reply "SAVED"
        "LOAD":   read both speeds back from EEPROM

        ANYTHING ELSE:
              reply "ERR:UNKNOWN"
              DO NOT CHANGE THE MOTORS
              (a stray character must never make the robot drive off)
```

Why BRAKE and not "speed 0": speed 0 switches the motor driver off and the robot
**coasts**. Brake shorts both motor terminals and it stops **now**.

> Not built, worth knowing: if the ESP32 loses power mid-run, the Nano keeps driving
> because nothing tells it to stop. A "brake if I have heard nothing for 3 seconds"
> failsafe would close that hole.

---

## 3. Hub — Track ESP32 (`firmware/hub/`)

Two jobs on two cores, so the web page can never stutter the LEDs.

```
START UP:
    LED strip on, all pixels off
    buzzer pin off, emergency button pin as input (pulled down)
    mount the flash filesystem (holds the web page)
    open the photo store:  microSD if a card is fitted, otherwise flash
    start Wi-Fi hotspot "MuseumHub" on channel 1, address 192.168.4.1
    start the captive-portal DNS (any address a phone asks for -> our page)
    start ESP-NOW on channel 1 and listen
    register the web addresses (/, /admin.html, /api/...)
    start the WEB JOB on core 0;  the SHOW JOB is the main loop on core 1


WEB JOB  (core 0, repeat forever):
    answer one DNS question
    answer one web request:
        GET  /                    -> the visitor page
        GET  /admin.html          -> the admin page
        GET  /api/state           -> everything we know, as JSON, every 1.5 s
        POST /api/checkin         -> a visitor asks to come in  (returns an id)
        GET  /api/enter           -> same, but for a phone-camera QR scan
        POST /api/approve?id=     -> staff lets them in
        POST /api/deny?id=        -> staff turns them away
        POST /api/emergency?on=   -> staff raises or clears the emergency
        POST /api/image/upload    -> the camera sends a photo (multipart)
        GET  /api/image?seq=      -> one stored photo
        GET  /api/gallery         -> the list of stored photos, newest first


SHOW JOB  (core 1, repeat forever, ~5 ms per pass):

    1. take every ESP-NOW message waiting in the inbox:
           LOCATION  ->  remember which station the robot is at, and whether it moves
           HEALTH    ->  remember that artifact's temperature / humidity / gas
           EMERGENCY ->  somebody else raised it: adopt it and pass it on

    2. IF the robot has said nothing for 2.5 s, stop calling it "moving"

    3. read the emergency button (debounced 50 ms):
           a PRESS            -> emergency ON  (latched)
           HELD for 3 seconds -> emergency OFF
       also adopt any change made from the admin page

    4. IF the emergency is on, wail the siren: 600 Hz / 1200 Hz, swapping every 400 ms

    5. tell everyone the emergency state:
           the moment it CHANGES  -> send it now, then 6 more times 60 ms apart
                                     (one lost packet must not strand the robot)
           otherwise              -> send it every 2 seconds anyway
                                     (so a robot that just rebooted catches up)

    6. paint the station LEDs:

           IF emergency:
               blink every pixel RED, twice a second

           ELSE IF we know which station the robot is at:
               that station's pixel is
                   GREEN   when the robot is STOPPED there and narrating   <-- #7
                   BLUE    when the robot is moving
               every other pixel is off

           ELSE
               all pixels off
```

> **On item 7:** this rule is already written and working on the Hub. The green light
> does not appear today only because the rail robot never sends `MSG_LOCATION`. Add
> step 8 of the robot's loop and the LED turns green with no Hub change at all.

---

## 4. Camera — ESP32-CAM (`firmware/cam/`)

Takes one picture of each exhibit while the robot is talking about it.

```
START UP:
    start the camera sensor (GC2145, RGB565, 320x240 — this sensor cannot make JPEGs
                             by itself, so we convert in software)
    listen on the trigger wire from the robot (GPIO13, 9600 baud, receive only)
    turn the flash LED off
    join the Hub's Wi-Fi "MuseumHub" as a normal client
    prepare an ESP-NOW broadcast so we can ping the Hub when a photo is ready

REPEAT FOREVER:
    read a command line, from either the USB console or the robot's wire
    (the SD card is optional; without one, photos still go to the Hub from memory)

ON "SHOOT <station>"  (this is what the robot sends):
    make sure we are on the Hub's Wi-Fi (join again if we dropped)
    throw away the stale frame the sensor has been holding
    grab a fresh frame
    convert it to a JPEG in memory
    keep it in memory as "the last photo" (so we can retry a failed send)
    IF an SD card is fitted, save a backup copy as /photos/IMG_####.JPG
    POST it to  http://192.168.4.1/api/image/upload?station=<station>
        as multipart form-data, field name "image"
    IF the Hub answers 200 OK:
        send a one-packet "image ready" ping over ESP-NOW

OTHER COMMANDS (for testing from a laptop):
    CAPTURE / SEND / WIFI / AUTO <ms> / STATUS / LIST / INIT / FLASH ON|OFF / HELP
```

The Hub checks every arriving photo before keeping it: the byte count must match, and
the file must start with `FF D8` and end with `FF D9` (that is what a real JPEG looks
like). A half-arrived photo is thrown away and the previous one is kept.

---

## 5. Face display — Arduino Mega 2560 + TFT (`firmware/display/`)

The robot's face. It welcomes visitors and shows how the robot "feels". It is a screen,
nothing more — it has no Wi-Fi and makes no decisions.

```
START UP:
    wake the TFT shield up, turn it sideways (480 x 320)
    show the WELCOME screen:  "SMART MUSEUM"  +  "Robot Display"
    wait about a second
    show the FACE

REPEAT FOREVER:
    animate the face:
        the eyes look slowly left and right
        the robot blinks now and then (both eyelids ease down and back up)
        the mouth matches the current emotion:
            HAPPY    smiling, the everyday tour face
            NEUTRAL  calm, while travelling between exhibits
            ALERT    wide-eyed, for an emergency
            SLEEPY   half-closed, when nothing is happening

    IF a text line arrives on the serial link (optional — only when a wire is fitted):
        "FACE <happy|neutral|alert|sleepy>"  -> change the emotion
        "STATION <n> <moving>"               -> show that exhibit's name for a while
        "EMERGENCY"                          -> red screen: "Follow the robot to the
                                                 nearest EXIT. Stay calm."
        "MENU" / "BOOT"                      -> the welcome screen again
        anything else                        -> ignore it

    (With no wire fitted the display simply keeps a friendly, blinking face. That is
     all we promised it would do.)
```

---

## 6. Artifact health — ESP32 per artifact (`firmware/health/`) — **TO BUILD (B)**

One small board watching one artifact. This is the "protecting cultural heritage" half
of our story, and it is the last missing feature.

```
START UP:
    set MY_STATION = the artifact I am watching (1..7)
    start the DHT11 temperature + humidity sensor
    set the MQ-135 gas sensor's analog pin as an input
        (use GPIO 32..39 — the other analog pins stop working when Wi-Fi is on)
    put Wi-Fi on channel 1 and start ESP-NOW
    warm up: the MQ-135 needs 24-48 hours of power before its readings settle,
             so switch these boards on the day before, not at the venue

REPEAT FOREVER:
    EVERY 3 SECONDS:
        read the temperature   (for example 26.4 degrees)
        read the humidity      (for example 61 percent)
        read the gas level     (a raw number 0..4095, not a real ppm)

        IF the sensor did not answer, skip this round and try again

        build one small packet:
            station   = MY_STATION
            tempX10   = temperature x 10     (26.4  ->  264, so we send no decimals)
            humidX10  = humidity x 10
            gasRaw    = the gas number
        broadcast it to everybody (the Hub is listening)
```

The Hub stores the newest reading per artifact and the admin page colours it. **The
thresholds live in the web page, not in this board** (`webapp/admin.html`):

| Colour | Meaning | Rule |
|---|---|---|
| green | fine | anything below the "watch" rule |
| yellow — watch | keep an eye on it | temp ≥ 28 °C, **or** humidity ≤ 35 %, **or** gas ≥ 300 |
| red — alert | tell a curator | temp > 31 °C, **or** humidity > 70 %, **or** gas > 500 |

Say it as a *trend against our threshold*, never as a calibrated ppm — an MQ-135 cannot
measure that, and judges respect the honest version.

---

## What each board does when the emergency button is pressed

| Board | What it does |
|---|---|
| Hub | latches the alarm, wails the siren, blinks every station LED red, tells everyone over ESP-NOW every 2 s (and 7 times fast on the change) |
| Robot | cuts the narration, cancels the waiting photo, drives — **ignores every card except the END card**, where it stops and stays stopped |
| Nano | just does what it is told: START, then STOP at the end |
| Camera | keeps working; the robot simply stops asking for photos |
| Display | red screen: "Follow the robot to the nearest EXIT" (when the wire is fitted) |
| Health | keeps reporting; nothing changes |

Clearing the alarm (hold the button 3 s, or the admin page) **does not** restart the
tour. The robot stays stopped so a person can put it back at the beginning, and the
START card begins the tour again — the same card as always. One card, one habit, nothing
new to remember on demo day.
