# knx_platformio_rgbcontroller
KNX RGB controller based on knx_platformio library

# RGB controller

Co-development with @MrWheel of a multi-channel KNX RGB controller.
Making use of the 16 PWM channels on an ESP32 this project aims to control multiple RGB Led light(strip)s.

The idea is to make a generic LED controller that is easy to operate using familiar physical KNX switches in the house
while allowing for more advanced settings per HomeKit.

## Road map

Currently 3 versions of Firmware and Application are planned to work on 2 versions of hardware.

### First F/W version (H/W v1) will support 

- 5 fixed sets of 3 GIOs per RGB channel (5 RGB channels)

- ON/OFF Set and Feedback (2 GroupObject - physical switch toggle/feedback)
- RGB color Set and Feedback (2 GroupObjects - Apple HomeKit (e.g. with Thinka))

### Second F/W version (H/W v1) will support

- Cycle through hard-coded pre-sets for favourite colors (1 GroupObject - physical switch long press)
- ETS Parameterized pre-sets

### Third F/W version (H/W v2) supports

Software Defined Color Channels (SDCC) to replace 5 fixed RGB channels for

  - RGB (3 physical channels)
  - RGBW (4 physical channels)
  - RGBWW (5 physical channels)

Supporting any combination of RGB/RGBW/RGBWW with a maximum of physical 16 channels devided in max 5 SDCC.

# Hardware

Initial hardware will support firmware versions one and two. 

An external 24v power supply will power the hardware. Hardware will provide 1 output per channel with (a number of) common ground connectors.
Version one will have 5 groups of 4 pins connectors, supporting up to 5 independent RGB channels. 

When switched off, the LEDs will have no power supplied to have a low standby power consumption.

# Terminology

Hardware: ESP32 based custom hardware designed by @MrWheel
Firmware: ESP32 software using knx_platformio library
Application: ETS application defined using XML and kaenx software
