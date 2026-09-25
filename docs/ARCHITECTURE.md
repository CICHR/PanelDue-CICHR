# Architecture

## Firmware layers

### Hardware

`src/Hardware/` contains display, touch, serial, flash, backlight, buzzer, reset and encoder support for the SAM3S/SAM4S PanelDue hardware.

### Controller communication

`src/PanelDue.cpp` handles the serial protocol, printer polling, object-model parsing, status changes and dispatch of received values to the UI.

### Object model

`src/ObjectModel/` stores machine objects such as axes, tools, heaters and spindles.

### File management

`src/FileManager.cpp` manages G-code and macro directory requests, navigation and file metadata.

### UI

`src/UI/` contains pages, widgets, popups, colour schemes, strings and event handling.

`DisplayField` objects track a changed flag. Normal refresh passes redraw only fields whose state changed. Full refresh is reserved for cases where the complete screen must be reconstructed.

### Persistent settings

`src/FlashData.*` stores PanelDue settings in internal flash, including touch/display configuration and configurable Home-page slot assignments.

## Display families

The build system selects the correct MCU and display definitions using `-DDEVICE=<target>`.

SAM3S4 is used for v2 targets and SAM4S4 for v3/integrated targets. Display-specific compile flags select 480×272, 800×480, CPLD and East Rising variants.

## Splash images

Editable PNG sources and generated PanelDue splash binaries are stored in `SplashScreens/`. `Tools/SplashScreenTool.py` converts between PNG and PanelDue RGB565/RLE format. The normal release image appends the resolution-matched splash to the raw application binary; the `-nologo` image does not.
