# Emergency — push-button node (PlatformIO project)

The physical emergency trigger. Keeps the robot **autonomous** (WRO rule 5.1.3 —
no remote control): the visitor/staff member presses a button, not an app.

**On the network:** broadcasts `MSG_EMERGENCY { FLAG_EMERGENCY_ACTIVE }` to the
broadcast address on press; broadcasts again (flag clear) on a second press/reset.
Every node reacts directly — no reliance on the Hub.

## Status
⬜ Skeleton. Implement in Phase 5.

## Planned wiring
| Part | ESP32 pin | Notes |
|------|-----------|-------|
| Big push button | GPIO… | to GND with INPUT_PULLUP; debounce in code |
| Status LED | GPIO… | shows armed/active |
| (optional) buzzer | GPIO… | local sound |

Build: `cd firmware/emergency && pio run -t upload`.
