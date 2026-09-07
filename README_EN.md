- [Chinese](README.md)
- [English](README_EN.md)

# Update List

- 2024/12/21
  1. Added support for zmk-studio (just refresh the left hand to use).
- 2024/10/24
  1. Modified power supply mode to reduce power consumption.
  2. Fixed the automatic shut-off feature for RGB power supply.
 
-2026/6/22
The keyboard now supports key remapping via DYA STUDIO. Chinese users should contact the seller to obtain the Chinese version of the DYA STUDIO installer. This PC software offers better key remapping functionality than ZMK Studio. Website: https://studio.dya.cormoran.works/ https://studio.dya.cormoran.works/

## Dongle and monitor builds

The `monitor` branch shares the existing Classic/YADS OLED theme settings but produces two
different topologies because the ZMK split-central role is selected at build time:

- `eyelash_sofle_central_dongle_oled.uf2`: normal USB/BLE HID dongle.
- `monitor_keyboard_left_central.uf2`: left keyboard half becomes the central, connects to the
  host, and broadcasts status using the Prospector v2.2.2 protocol on channel 1.
- `monitor_display_receiver.uf2`: the original receiver hardware becomes a display-only BLE
  observer and does not output keyboard HID reports.
- `eyelash_sofle_peripheral_right_nice_view.uf2`: right half used by the monitor topology.

The monitor displays both half batteries, layer, modifiers, WPM, and USB/BLE state. The existing
DYA `display_theme` setting continues to select the OLED layout.

> If your keyboard was updated before October 24, please update to the latest firmware.
> 
---
# Contact Me

For 3D printed model files or any issues and malfunctions with the keyboard, please contact 380465425@qq.com

# Sofle Keymap


<img src="keymap-drawer/eyelash_sofle.svg" >
