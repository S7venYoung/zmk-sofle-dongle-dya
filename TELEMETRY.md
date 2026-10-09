# Cube fighting telemetry v1

Sofle `fighting-theme` keeps its original side-specific physical-key statistics:
5-second sliding window, 64 presses per side, WPM = round(count * 12 / 5).
At most ~154 WPM can be represented by the existing history. Key releases,
host auto-repeat and encoder motion are not counted. Left/right classification
uses this Sofle's transform: position <64 and position %13 <6 is left.
Do not reuse that mapping for another keyboard without adapting it.

Broadcast at 200ms using a separate legacy-format advertising set, not a
connection. Existing HID advertising and OLED animation stay intact. Bluetooth
initialization failures retry, without blocking input. Payload is manufacturer
type 0xff, exactly 17 bytes:

| Offset | Meaning |
| --- | --- |
| 0..4 | ff ff ab ce 01 |
| 5 | flags: bit0 WPM valid, bit1 left battery valid, bit2 right battery valid |
| 6..9 | keyboard identity, little endian |
| 10,11 | left/right WPM |
| 12,13 | left/right battery (0..100, use validity bits) |
| 14 | USB HID modifier bitmask |
| 15,16 | wrapping little-endian sequence |

Scan-response name: CUBE-FIGHT. This new packet is separate from the original
26-byte ff ff ab cd Prospector advertisement. Cube accepts the first identity,
expires it after 3 seconds, and never substitutes total WPM for side WPM.
No authentication: nearby compatible broadcasters can be observed; this is
display-only data and must not control typing or configuration.

Peripheral battery source0/source1 follow the original fighting theme's
left/right convention; verify bindings on hardware. 0% is valid, not unknown.
Only the central dongle needs this telemetry code; peripheral firmware remains
unchanged. Both ends still need Actions compilation and radio coexistence
testing. The Street Fighter RGB animation theme is not yet implemented.
