# Splash screen / boot screen

PanelDue CICHR uses a bitmap appended to the end of the normal firmware image. The `-nologo.bin` image does not contain this appended bitmap.

## Editable source images

```text
SplashScreens/SplashScreen-CICHR-480x272.png
SplashScreens/SplashScreen-CICHR-800x480.png
```

Use `480×272` for 4.3" displays and `800×480` for 5"/7" displays.

## Edit the artwork

Open the PNG in any normal raster editor and keep its exact pixel dimensions. RGB artwork is converted to the 16-bit RGB565 format used by the display.

Recommended workflow:

1. Edit the PNG.
2. Save it with the same filename.
3. Run `MAKE_SPLASH.bat`.
4. Build the firmware again.

## Convert PNG to PanelDue binary

Windows helper:

```bat
MAKE_SPLASH.bat
```

The helper installs Pillow if required and runs:

```text
Tools/SplashScreenTool.py
```

Direct commands are also possible:

```text
py Tools\SplashScreenTool.py encode SplashScreens\SplashScreen-CICHR-800x480.png SplashScreens\SplashScreen-CICHR-800x480.bin
```

Decode an existing PanelDue splash back to PNG:

```text
py Tools\SplashScreenTool.py decode SplashScreens\SplashScreen-CICHR-800x480.bin test.png
```

The converter uses the format expected by `UTFT::drawCompressedBitmapBottomToTop`: two little-endian 16-bit values for width/height followed by RGB565 run-length pairs.

## Which splash is packaged

The build selects:

```text
SplashScreen-CICHR-480x272.bin
```

for `v2-4.3` and `v3-4.3`.

All 800×480 targets use:

```text
SplashScreen-CICHR-800x480.bin
```

## Display time

The startup image remains visible until touched or until this timeout expires:

```cpp
SplashScreenHoldTime
```

in:

```text
src/Configuration.hpp
```

Default:

```text
5000 ms
```

There is no separate image-based loading screen after the splash. When the splash ends, the firmware draws the normal interface and begins/continues machine communication.
