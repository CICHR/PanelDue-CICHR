# Building PanelDue CICHR v1.0.0

## Windows: one target

```bat
BUILD_ONE.bat <target>
```

Example:

```bat
BUILD_ONE.bat v3-5.0
```

Supported targets:

```text
v2-4.3
v2-5.0
v2-7.0
v2-7.0c
v3-4.3
v3-5.0
v3-7.0
v3-7.0c
5.0i
7.0i
```

If no target is supplied, the script builds `v3-5.0`.

## Output files

Each target folder contains only two firmware binaries.

Example for `v3-5.0`:

```text
output\v3-5.0\PanelDue-CICHR-v1.0.0-v3-5.0.bin
output\v3-5.0\PanelDue-CICHR-v1.0.0-v3-5.0-nologo.bin
```

The normal `.bin` has the target-appropriate boot splash appended. The `-nologo.bin` file is the raw application image without splash data.

Use the normal `.bin` unless you intentionally need a firmware image without a packaged splash.

## Windows: all targets

```bat
BUILD_ALL.bat
```

The script clears `output\`, builds all supported targets and creates one folder per target with the same two-file naming scheme.

## Required tools

`BUILD_ONE.bat` checks for:

- CMake,
- Ninja,
- Git,
- Arm GNU Toolchain (`arm-none-eabi`).

On supported Windows systems it can use `winget` for missing tools. It validates the C++ toolchain before compiling.

## Dependencies

When not already present, the builder retrieves pinned revisions of:

- Duet3D `base64`,
- Duet3D `RRFLibraries`,
- Duet3D `qoi`.

The dependency cache is kept in the Windows temporary directory and copied into `lib/` for the build.

## Splash source used by the build

4.3" targets use:

```text
SplashScreens\SplashScreen-CICHR-480x272.bin
```

5" and 7" targets use:

```text
SplashScreens\SplashScreen-CICHR-800x480.bin
```

Edit the matching PNG and run `MAKE_SPLASH.bat` before building if you want custom artwork. See [SPLASH_SCREEN.md](SPLASH_SCREEN.md).

## Manual CMake build

A manual build requires these libraries:

```text
lib/base64
lib/librrf
lib/qoi
```

Configure:

```text
cmake -S . -B build -G Ninja -DDEVICE=v3-5.0 -DCROSS_COMPILE=/path/to/arm-none-eabi-
```

Compile the application:

```text
cmake --build build --target paneldue.elf --parallel
```

`BUILD_ONE.bat` is the recommended release build because it also creates the two correctly named upload images and appends the matching splash image.

## Clean build behaviour

The target-specific CMake directory is removed before configuration, so every `BUILD_ONE.bat` invocation starts with a clean target build.
