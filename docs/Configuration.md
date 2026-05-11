# Configuration

mag-usb reads an optional `config.toml` at startup. It searches for the configuration file in the following order:
1. `/etc/mag-usb/config.toml`
2. `config.toml` in the current working directory.

If neither file is found, defaults are used. Invalid values are ignored or reduced to defaults. Command-line arguments always override both default values and those found in any configuration file.

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
- `cc_x`, `cc_y`, `cc_z` (int) — Cycle counts. Typical values: 200, 400. Default: 400 (if set in config.toml).
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
- `pipe_in_path` (string) — Path to control pipe (writer side). Default: `/run/mag-usb/magctl.fifo`.
- `pipe_out_path` (string) — Path to data pipe (reader side). Default: `/run/mag-usb/magdata.fifo`.

Note: When `use_pipes=true`, the program will create the pipes if they do not exist with `0666` permissions.

### [websocket]
- `enable` (bool) — Enable the WebSocket output server. Default: false.
- `bind_address` (string) — Server bind address. Default: `0.0.0.0`.
- `port` (int) — Server port. Default: 8765.
Note: This section is active only when built with `-DENABLE_WEBSOCKET=ON`.

## Example
```toml
# mag-usb Configuration File
# Lines starting with # are comments.
# Inline comments (Data followed by hashmark) not allowed.
[node_information]
maintainer = "Dave Witten, KD0EAG"
maintainer_email = "wittend@wwrinc.com"

[node_location]
# Tools:
#   lat/long: Google Maps or GNSS device.
#   Altitude: Optional, or Use GNSS device, or find yourself in Google Earth and look at the status bar at the bottom.
#   Grid square conversion: https://www.giangrandi.org/electronics/radio/qthloccalc.shtml or similar.
latitude = "38.92263"
longitude = "-92.29831"
elevation = "230 M"
grid_square = "EM38uw"

[output]
# Write logfiles to log_output_path..
write_logs = false
# create logfiles in this location.
log_output_path = "./logs"
# create this location if it does not esist.
create_log_path_if_empty = true

# Use named pipes for IPC.
use_pipes = false
# Pipe paths (if use_pipes is true).
pipe_in_path = "/run/mag-usb/magctl.fifo"
pipe_out_path = "/run/mag-usb/magdata.fifo"

[websocket]
# Enable WebSocket output server.
enable = false
# Bind address and port for WebSocket clients.
bind_address = "0.0.0.0"
port = 8765

[i2c]
# Use external USB to I2C device.
use_I2C_converter = true
# Path to the I2C device.
portpath = "/dev/ttyACM0"
# I2C bus number (for non-Pololu setups).
bus_number = 1
# Scan I2C bus on startup.
scan_bus = false

[magnetometer]
# Magnetometer I2C address (hex format supported).
address = 0x23
# Cycle Count registers (200 or 400 typically).
cc_x = 400
cc_y = 400
cc_z = 400
# Gain values.
gain_x = 150.0
# TMRC Rate register value (hex format).
tmrc_rate = 0x96
# Number of samples register value.
nos_reg_value = 60
# DRDY delay in microseconds.
drdy_delay = 10
# Sampling mode: "POLL" or "CMM".
sampling_mode = "POLL"
# CMM sample rate (Hz)
cmm_sample_rate = 400
# Read back cycle count registers after setting.
readback_cc_regs = false

# Magnetometer Orientation in degrees relative to default (See hardware setup docs).
# Plus and minus 180 are equivalent. Plus and minus 90 are different!
[mag_orientation]
# Allowed values: -180, -90, 0, 90, 180
mag_translate_x = 0
mag_translate_y = 0
mag_translate_z = 0

[temperature]
# Remote temperature sensor I2C address
remote_temp_address = 0x1F
```
