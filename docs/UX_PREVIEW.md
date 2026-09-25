# UX preview

The repository includes a local browser preview for the 800×480 interface.

## Start

On Windows:

```bat
UX_PREVIEW.bat
```

or open directly:

```text
preview/index.html
```

## Preview data

Edit:

```text
preview/preview-config.js
```

The configuration can simulate:

- machine name and status,
- current file,
- axis position,
- homing state,
- heaters and current temperatures,
- configurable Home slots,
- macros,
- fans and their names/values,
- outputs and their names/values,
- elapsed time and ETA,
- progress,
- print speed,
- settings values.

## Purpose

The preview is intended for UI layout work without repeatedly flashing hardware. It models the 800×480 screen geometry and control layout. Hardware-specific font rasterization, touch behaviour and display-controller timing can still differ from a desktop browser.
