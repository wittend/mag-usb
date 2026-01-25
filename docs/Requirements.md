# mag-usb Requirements

This document defines the functional, non-functional, and delivery requirements for a clean-room reimplementation of the mag-usb project.
It is a specification document, not an implementation guide.
It is intended as a start-to-finish blueprint: hardware, software, build, runtime behavior, and documentation.

## 1. Scope and goals
- Provide a Linux command-line utility that reads a PNI RM3100 3-axis magnetometer via a Pololu USB-to-I2C adapter.
- Emit time-stamped magnetic field vectors in a JSON-lines format for downstream ingestion.
- Support a simple TOML configuration file, with command-line overrides.
- Provide lightweight diagnostics (adapter check, I2C bus scan, sensor presence checks).
- Keep implementation portable C (C11) with minimal dependencies.

Non-goals:
- Windows support.
- Arbitrary-angle orientation or full magnetometer calibration (soft/hard iron compensation).
- GUI or web interface.

## 2. Supported platforms and environment
- Target OS: Linux.
- Expected device nodes: USB CDC ACM (`/dev/ttyACM*`).
- macOS: untested and optional.
- SBCs (Raspberry Pi class): not primary target; use Pololu adapter instead of native GPIO/I2C libraries.

## 3. Hardware requirements
- Pololu Isolated USB-to-I2C Adapter:
  - Product 5396 or 5397.
- RM3100-based magnetometer board.
- Wiring:
  - SDA to SDA, SCL to SCL, GND to GND, optional +5V if required by board.
  - Confirm I2C pull-ups exist.
- Optional: MCP9808 temperature sensor for remote temperature readings.

## 4. System behavior and functional requirements

### 4.1 Program startup
- Load defaults for all parameters.
- Load configuration file from:
  1) `/etc/mag-usb/config.toml`
  2) `config.toml` in current working directory
- If the first file fails to load or does not exist, fall back to local file.
- Ignore or clamp invalid config values to defaults.
- Parse command-line arguments after config load; CLI overrides config and defaults.
- If `-P` is passed, print active settings and exit before hardware initialization.

### 4.2 Command-line interface (CLI)
Provide the following options:
- `-B <reg mask>`: BIST (not implemented, but option exists).
- `-C`: read back cycle count registers before sampling.
- `-c <count>`: set cycle count for X/Y/Z; valid range 1..800 (decimal).
- `-D <rate>`: set CMM sample rate (integer).
- `-g <mode>`: sampling mode (POLL=0, CMM=1).
- `-O <path>`: Pololu adapter device path (default `/dev/ttyACM0`).
- `-P`: show current settings and exit.
- `-Q`: verify Pololu adapter presence and exit.
- `-M`: verify magnetometer presence and version and exit.
- `-S`: scan I2C bus and exit.
- `-T`: verify temperature sensor presence and version and exit.
- `-u`: enable named pipes for output.
- `-i <path>`: input named pipe path.
- `-o <path>`: output named pipe path.
- `-V`: print version and exit.
- `-h`/`-?`: print help and exit.

### 4.3 I2C adapter handling
- Verify adapter availability at the configured device path with timeout.
- Validate adapter is a supported Pololu device.
- If adapter is missing or invalid, exit with a clear error.
- If `-Q` is set, only validate adapter and exit 0/1.

### 4.4 Sensor handling
- Support RM3100 magnetometer at a configurable I2C address (default from build config).
- Support MCP9808 temperature sensor at configurable I2C address (default 0x1F).
- Provide diagnostic checks:
  - `-M`: verify magnetometer.
  - `-T`: verify temperature sensor.
  - `-S`: scan I2C bus and report devices.
- Initialize magnetometer registers before sampling.

### 4.5 Sampling and output
- Primary data output: JSON lines, one per sample.
- Output schema:
  - `ts` (string): UTC timestamp, format `DD Mon YYYY HH:MM:SS` (RFC-2822-like without timezone).
  - `rt` (number): remote temperature in deg C.
  - `x`, `y`, `z` (number): magnetometer values in nT, formatted with 3 decimal places.
- Convert RM3100 raw counts to microTesla then multiply by 1000 to nT.
- Apply orientation rotations (see 4.6) before output.
- Log and diagnostic messages may be interleaved with JSON lines on the same stream.

### 4.6 Orientation rotations
- Support 90-degree increments about X, Y, Z axes using right-hand rule.
- Allowed values: -180, -90, 0, 90, 180.
- Apply rotations in order X, then Y, then Z.
- Invalid values fall back to 0.

### 4.7 Named pipes and logging
- If `use_pipes` is enabled or `-u` flag is provided:
  - Create FIFOs if missing with mode 0666.
  - Default paths (from sample config):
    - Input: `/run/mag-usb/magctl.fifo`
    - Output: `/run/mag-usb/magdata.fifo`
  - Open output pipe as non-blocking write, input as non-blocking read.
- If `write_logs` is enabled:
  - Write JSON lines to log files in `log_output_path`.
  - Create directory if `create_log_path_if_empty` is true.

### 4.8 WebSocket output (optional)
- Provide a build flag `ENABLE_WEBSOCKET` to include WebSocket server support.
- When enabled and configured, broadcast each JSON output line to all connected clients.
- Allow runtime configuration of bind address and port.
- Accept multiple concurrent clients; send text frames.

### 4.9 Configuration keys
TOML sections and keys to support:

[node_information]
- `maintainer` (string)
- `maintainer_email` (string)

[node_location]
- `latitude` (string)
- `longitude` (string)
- `elevation` (string)
- `grid_square` (string)

[i2c]
- `use_I2C_converter` (bool)
- `portpath` (string)
- `bus_number` (int)
- `scan_bus` (bool)

[magnetometer]
- `address` (int, decimal or hex)
- `cc_x`, `cc_y`, `cc_z` (int)
- `gain_x`, `gain_y`, `gain_z` (double)
- `tmrc_rate` (int, decimal or hex)
- `nos_reg_value` (int)
- `drdy_delay` (int)
- `sampling_mode` (string "POLL" or "CMM")
- `cmm_sample_rate` (int)
- `readback_cc_regs` (bool)

[mag_orientation]
- `mag_translate_x`, `mag_translate_y`, `mag_translate_z` (int)

[temperature]
- `remote_temp_address` (int, decimal or hex)

[output]
- `write_logs` (bool)
- `log_output_path` (string)
- `create_log_path_if_empty` (bool)
- `use_pipes` (bool)
- `pipe_in_path` (string)
- `pipe_out_path` (string)

[websocket]
- `enable` (bool)
- `bind_address` (string)
- `port` (int)

## 5. Non-functional requirements
- Language: C11; avoid compiler-specific extensions without guards.
- Dependencies: none beyond standard POSIX/Linux system libraries.
- Build tooling: CMake 3.22+.
- Warnings: build with `-Wall -Wextra -Wpedantic`; allow optional `-Werror`.
- Runtime stability: exit with meaningful error messages for adapter/sensor failures.
- Performance: stable sampling, acknowledging jitter from host scheduling and JSON printing.

## 6. Build, test, and packaging

### 6.1 Build
- Provide a top-level `CMakeLists.txt`.
- Build targets:
  - `mag-usb` (main CLI).
  - `i2c-pololu-tests` (unit tests).
- Provide Debug/Release builds via CMake or IDEs (e.g., CLion).

### 6.2 Tests
- Integrate unit tests with CTest.
- Command line:
  - `cmake -S . -B build -DBUILD_TESTING=ON`
  - `cmake --build build --target i2c-pololu-tests`
  - `(cd build && ctest --output-on-failure)`

### 6.3 Installation artifacts
- Include a sample `config.toml`.
- Provide udev rule for Pololu adapter in `install/99-PololuI2C.rules`.
- Document installation of udev rule and permissions.

## 7. Documentation deliverables
Required documentation pages:
- Getting started and build steps.
- Hardware wiring and udev setup.
- Configuration reference and examples.
- Output data format.
- Orientation and axes explanation.
- Troubleshooting guide.
- Development guide (build matrix and coding patterns).

## 8. Acceptance criteria
- Build succeeds with CMake 3.22+ on Linux using GCC or Clang.
- `mag-usb -h` shows all documented CLI flags.
- `mag-usb -P` prints effective configuration values.
- `mag-usb -Q` validates adapter and exits.
- JSON output matches schema and units, with orientation rotations applied.
- Config file parsing and CLI overrides behave as specified.
- Named pipes and logging operate per configuration with correct permissions.
- Unit tests compile and run via CTest.
