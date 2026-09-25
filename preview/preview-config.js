window.PANELDUE_PREVIEW_CONFIG = {
  machineName: "CichrCore",
  status: "Printing",
  printFile: "0:/gcodes/PETG_Filter_Big_2h35min.gcode",
  machineMode: "FFF",
  position: { x: 133.0, y: 443.3, z: 35.2 },
  axes: ["X", "Y", "Z"],
  homed: { x: false, y: true, z: true },
  temperatures: [
    { type: "bed", index: 0, label: "Bed", icon: "bed", current: 61.2, active: 70, standby: 0 },
    { type: "tool", index: 0, label: "Tool 0", icon: "nozzle", current: 100.0, active: 100, standby: 0 },
    { type: "tool", index: 1, label: "Tool 1", icon: "nozzle", current: 100.0, active: 0, standby: 0 },
    { type: "tool", index: 2, label: "Tool 2", icon: "nozzle", current: 100.0, active: 0, standby: 0 }
  ],
  homeSlots: [
    { type: "empty" },
    { type: "fan", index: 2 },
    { type: "fan", index: 3 },
    { type: "output", index: 7 },
    { type: "macro", index: 0 },
    { type: "empty" },
    { type: "empty" },
    { type: "empty" }
  ],
  macros: [
    { name: "Load filament", file: "Load filament.g" },
    { name: "Unload filament", file: "Unload filament.g" }
  ],
  fans: [
    { name: "Part cooling", value: 0 },
    { name: "Chamber circulation", value: 0 },
    { name: "Filter fan", value: 100 },
    { name: "Electronics fan", value: 100 }
  ],
  outputs: [
    { name: "Light", value: 0 },
    { name: "Aux output", value: 0 },
    { name: "Vacuum", value: 0 },
    { name: "Air valve", value: 0 },
    { name: "Output 4", value: 0 },
    { name: "Output 5", value: 0 },
    { name: "Heater relay", value: 8 },
    { name: "Cabinet light", value: 20 }
  ],
  elapsed: "0:04:49",
  eta: "2h 5m",
  progress: 0,
  speed: 100,
  bed: { xMin: 0, xMax: 300, yMin: 0, yMax: 200, circular: false },
  settings: {
    language: "English",
    baud: 57600,
    volume: 3,
    colour: "Grey",
    dimming: "Dim",
    infoTimeout: 4,
    screensaver: 0,
    babystep: "0.05",
    feedrate: 3000,
    heaterCombine: "Combined",
    logLevel: "Info",
    invertZ: false,
    ip: "192.168.1.52"
  }
};
