# FluidNC Usermod
FluidNC is a firmware for controlling CNC machines.
https://github.com/bdring/FluidNC

This usermod will display a preset based off the current FluidNC state as well as a position overlay in the Jog and Run states.

## Usage
Compile the source with the buildflag  `-D USERMOD_FLUIDNC`.

## Setup

### Settings 
- Enabled - Checkbox to enable the usermod.
- FluidNC URL - The URL/IP address of the FluidNC controller.
- FluidNC Port - The TCP/Telnet port of the FluidNC controller (typically 23).
- CNC X Width Mm - Usable X width in mm of the CNC machine.
- CNC X Position LED Width - Number of LEDs to overlay the position.  Use an odd number.
- CNC X Position Offset Mm - Offset in mm from the leftmost LED to the endmill.
- CNC X Position Reverse - Checkbox if the LED strip starts at the max X position.
- LED Strip Length Mm - Length in mm of the LED strip from the middle of first LED to middle of last LED.

### Presets
Create presets with the following IDs for each state:
1. Idle
2. Home
3. Alarm
4. Hold
5. Run
6. Jog
7. Unknown

## Changelog
- 2024-01-20 - Jason Yeager - Initial version.