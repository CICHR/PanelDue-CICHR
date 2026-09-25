# Flashing PanelDue CICHR v1.0.0

Always use firmware built for the exact PanelDue target. A wrong target can give a blank or unusable display and may require bootloader recovery.

## Normal file

For normal use choose the file without `-nologo`:

```text
PanelDue-CICHR-v1.0.0-<target>.bin
```

Example:

```text
PanelDue-CICHR-v1.0.0-v3-5.0.bin
```

This file contains the application and the correct boot splash for the selected display resolution.

## No-logo file

```text
PanelDue-CICHR-v1.0.0-<target>-nologo.bin
```

This is the application image without appended splash data. It is useful for development and custom flash workflows. It is not a different feature set.

## Duet update

Use the standard PanelDue firmware update process for your Duet/RepRapFirmware setup and select the target-specific binary above.

Official reference:

https://docs.duet3d.com/en/User_manual/RepRapFirmware/Updating_PanelDue

## Direct bootloader programming

When the PanelDue is in bootloader mode, BOSSA/bossac may be used to program the firmware. The exact port and bootloader procedure depend on the PanelDue hardware revision.

Typical form:

```text
bossac -e -w -v -b <firmware.bin> -R -p <serial-port>
```

## Before flashing

Confirm:

1. PanelDue hardware generation is correct.
2. Display size/type is correct.
3. The filename target matches the hardware.
4. For normal use you selected the file without `-nologo`.
