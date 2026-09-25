# Contributing

## Development workflow

1. Build the target you use on hardware before making UI changes.
2. Use `UX_PREVIEW.bat` for 800×480 layout work.
3. Keep target-specific assumptions out of shared UI code where possible.
4. Verify that text fits both the intended popup/button and the supported display size.
5. Avoid unnecessary full-display refreshes; update only fields whose displayed state changed.
6. Run a clean build for every target affected by display or hardware-level changes.

## Code constraints

The firmware runs on SAM microcontrollers with limited flash and RAM. Avoid runtime heap allocation. Existing UI objects are normally allocated during initialization and reused.

## Pull requests

Include:

- affected PanelDue target(s),
- RepRapFirmware version used for testing,
- a short reproduction description for bug fixes,
- screenshots for visible UI changes when practical.
