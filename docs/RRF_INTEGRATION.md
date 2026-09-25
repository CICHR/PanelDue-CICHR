# RepRapFirmware integration

PanelDue CICHR is designed primarily for RepRapFirmware 3.x and communicates over the PanelDue serial channel. The default serial rate is `57600 baud`.

## Object-model data

The firmware requests and processes data for:

- boards,
- fans,
- heat,
- job state,
- move/axes/extruders,
- network,
- sensors,
- spindles,
- state,
- tools,
- volumes.

The file manager separately requests G-code files, file information and macros.

## Fans and names

Fan values come from the RRF `fans[]` object. The display reads `fans[].name` and uses it when it is non-empty.

Example RRF configuration:

```gcode
M950 F0 C"fan0" Q500
M106 P0 C"Electronics"
```

The visible label can therefore be `Electronics` instead of `F0`.

## General-purpose outputs

RRF exposes configured GPIO/PWM output state under:

```text
state.gpOut[]
```

Each configured entry provides its current PWM value and, in newer RRF releases, frequency. Sparse/unconfigured entries may be null.

PanelDue CICHR requests the `state` object without RRF's PanelDue-only `p` filter so GP-output data required by this interface is present. It also makes a small unfiltered `state` live request periodically so output values remain current even when they are changed somewhere other than this touchscreen.

A typical configuration is:

```gcode
M950 P0 C"out1"
M950 P7 C"!out4"
M42 P0 S0
M42 P7 S0
```

When a valid `state.gpOut[n].pwm` value is received, output `n` becomes available in the UI. It is not removed merely because another object-model response is shorter.

### Output labels

Unlike fans, `state.gpOut[]` does not provide a separate friendly `name` field. For each detected output, PanelDue CICHR asks RepRapFirmware for:

```gcode
M950 Pn
```

and uses the assigned pin name returned by RRF as the label where possible. For example `M950 P7 C"!out4"` is displayed as `out4`.

If no usable pin label can be obtained, the fallback is `Output n`.

## Output troubleshooting

If an output is configured but does not appear, verify from the Duet console:

```gcode
M409 K"state" F"vn"
```

Look for a non-null entry under `gpOut` at the expected index.

Also run:

```gcode
M950 P0
M950 P1
```

or the output numbers you actually use. RRF should report the configured pin assignment.

The output must be created with `M950 Pn C"pin"`; a physical board pin used directly for another subsystem is not automatically a GP output.

## Macros

The macro browser reads the Duet macro directory. Root-level macros are also available to configurable Home quick slots, and the displayed tile text follows the macro filename.

## Home quick-slot storage

Persistent quick-slot encoding uses:

```text
0x00         automatic / empty
0x10..0x17   fan 0..7
0x20..0x27   output 0..7
0x30..0x37   root macro 0..7
```

Assignments are stored in PanelDue flash memory with the display settings.

## Commands sent by the UI

Normal G-code is used for operations such as:

- homing,
- jogging,
- temperature changes,
- fan/output changes,
- manual extrusion,
- speed and extrusion-factor adjustment,
- file selection and printing,
- macro execution.
