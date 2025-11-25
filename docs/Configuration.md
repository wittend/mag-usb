# Configuration

mag-usb reads an optional `config.toml` from the current working directory at startup. If the file is missing, defaults are used. Invalid values are ignored or reduced to defaults.

Tip: Run `./mag-usb -P` to print the active settings and exit.

## File format
- Syntax: TOML (key = value). Lines beginning with `#` are comments.
- Sections use `[section_name]` headers.

## Sections and keys
Below is the complete reference of supported sections/keys, their types, defaults, and notes.

### [node_information]
- `maintainer` (string)
- `maintainer_email` (string)

Defaults: empty strings.

### [node_location]
- `latitude` (string) — free‑form (e.g., "38.92263").
- `longitude` (string)
- `elevation` (string) — free‑form (e.g., "230 m").
- `grid_square` (string) — e.g., "EM38uw".

Defaults: empty strings.

### [i2c]
- `use_I2C_converter` (bool) — Use Pololu USB‑to‑I²C adapter. Default: true (when built with USE_POLOLU=TRUE).
- `portpath` (string) — Adapter device path. Default: `/dev/ttyACM0`.
- `bus_number` (int) — Linux I²C bus number for non‑Pololu setups. Default: 1.
- `scan_bus` (bool) — Probe for devices on startup. Default: false.

Notes:
- If `use_I2C_converter=true`, `portpath` must be accessible to the process (see udev notes in Hardware‑Setup.md).

### [magnetometer]
- `address` (int, decimal or hex `0xNN`) — RM3100 I²C address. Default: build‑time default from `RM3100_I2C_ADDRESS`.
- `cc_x`, `cc_y`, `cc_z` (int) — Cycle counts. Typical values: 200, 400. Default: 200.
- `gain_x`, `gain_y`, `gain_z` (double) — Gains. Default: 150.0.
- `tmrc_rate` (int, decimal or hex) — TMRC register value. Default: 0x96.
- `nos_reg_value` (int) — Number‑of‑samples register value. Default: 60.
- `drdy_delay` (int) — DRDY delay in microseconds. Default: 10.
- `sampling_mode` (string) — `"POLL"` or `"CMM"`. Default: `"POLL"`.
- `cmm_sample_rate` (int) — CMM sample rate (Hz). Default: 400.
- `readback_cc_regs` (bool) — Read back CC registers after setting. Default: false.

### [mag_orientation]
Defines fixed 90°‑increment rotations applied to the measured vector before printing.
- `mag_translate_x` (int) — Allowed values: -180, -90, 0, 90, 180.
- `mag_translate_y` (int) — Allowed values: -180, -90, 0, 90, 180.
- `mag_translate_z` (int) — Allowed values: -180, -90, 0, 90, 180.

Rules:
- Missing/invalid values default to 0.
- `180` and `-180` are treated equivalently.
- Rotations are applied in the order X, then Y, then Z, using right‑hand rule conventions.
- See Orientation-and-Axes.md for visuals and examples.

### [temperature]
- `remote_temp_address` (int, decimal or hex) — MCP9808 temperature sensor address. Default: 0x1F.

### [output]
- `write_logs` (bool) — Write logs to files. Default: false.
- `log_output_path` (string) — Path for log files. Default: `./logs` (when logging enabled).
- `create_log_path_if_empty` (bool) — Create `log_output_path` if missing. Default: true.
- `use_pipes` (bool) — Use named pipes for IPC. Default: false.
- `pipe_in_path` (string) — Path to control pipe (writer side). Default: none.
- `pipe_out_path` (string) — Path to data pipe (reader side). Default: none.
- `publish MQTT` (bool) — Publish output lines to an MQTT broker. Default: false.
- `MQTT_broker_URL` (string) URL for MQTT broker. Default: none. 

Note: When `use_pipes=false`, pipe paths are ignored.  [Actually, pipe communication is turned off in this version].

## Example
```
[node_information]
maintainer = "Jane Doe"
maintainer_email = "jane@example.org"

[node_location]
latitude = "38.92263"
longitude = "-92.29831"
elevation = "230 m"
grid_square = "EM38uw"

[i2c]
use_I2C_converter = true
portpath = "/dev/ttyACM0"
scan_bus = false

[magnetometer]
address = 0x23
cc_x = 400
cc_y = 400
cc_z = 400
gain_x = 150.0
gain_y = 150.0
gain_z = 150.0
tmrc_rate = 0x96
nos_reg_value = 60
drdy_delay = 10
sampling_mode = "POLL"
cmm_sample_rate = 400
readback_cc_regs = false

[mag_orientation]
mag_translate_x = 90
mag_translate_y = 0
mag_translate_z = -90

[temperature]
remote_temp_address = 0x1F

[output]
write_logs = false
log_output_path = "./logs"
create_log_path_if_empty = true
use_pipes = false
```