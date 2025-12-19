# mag-usb
 
Version: 0.0.6

Official documentation -- https://mag-usb.readthedocs.io/

mag-usb is a Linux command‑line utility for reading a PNI RM3100 3‑axis magnetometer via a USB‑to‑I²C adapter (Pololu 5396/5397). It outputs time‑stamped magnetic field vectors and supports optional configuration via a simple TOML file.

The RM3100 boards were developed for the HamSCI Personal Space Weather Station (PSWS) TangerineSDR and Grape monitors, but mag-usb can be used as a standalone low‑cost geomagnetic field logger.

Key points:
- Portable C (no heavyweight dependencies).
- Designed to use the Pololu Isolated USB‑to‑I²C Adapter family.
- Works on typical Linux hosts with USB 2.0. Raspberry Pi‑class devices may work but are not the current target.
- Windows is not supported. macOS may work but is untested.

## Features
- Reads RM3100 magnetometer X/Y/Z and prints JSON lines including an RFC‑2822 timestamp.
- Configurable cycle counts, gains, and sampling parameters.
- Optional orientation translations in 90° increments about X/Y/Z, set in config.toml.
- Support for concurrent output to `stdout` and named pipes for IPC (local monitoring/dashboards).
- Convenience flag to print current settings (`-P`).
- Optional diagnostics: verify devices, scan I²C bus, etc.

## Build (using CMake)

Plain CMake:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target mag-usb
```

CLion IDE (convenient if you happen to have it):
- Open the project; CLion generates Debug and Release profiles under cmake-build-debug and cmake-build-release.
- Build the target `mag-usb` from the chosen profile.

## Quick start
- Connect the Pololu USB‑to‑I²C adapter and your RM3100 board.
- Run with your adapter device path (default is `/dev/ttyACM0`):
```
./build/mag-usb -O /dev/ttyACM0 -Q
```
Flags shown here:
- `-O` sets the adapter device path.
- `-Q` verifies adapter presence.

To print current settings (and exit):
```
./build/mag-usb -P
```
The program follows Linux filesystem conventions and looks for its configuration file in the following order:
1. `/etc/mag-usb/config.toml`
2. `config.toml` in the current working directory.

If neither is found, reasonable default values are used. Command-line arguments always overrule both default values and those found in any configuration file.

## Configuration
A simple TOML configuration file is supported. The program searches for it in `/etc/mag-usb/config.toml` first, then in the local directory. See:
- docs/Configuration.md — all keys, defaults, and examples.
- docs/Orientation-and-Axes.md — how the 90° orientation translations work.

Example orientation section:
```
[mag_orientation]
mag_translate_x = 90
mag_translate_y = 0
mag_translate_z = -90
```
Allowed values: `-180, -90, 0, 90, 180` (±180 are equivalent). Invalid/missing values default to 0.

## Output format
Each line is JSON with at least:
```
{ "ts": "DD Mon YYYY HH:MM:SS", "x": NNN.NNN, "y": NNN.NNN, "z": NNN.NNN }
```
- Units: nanoTesla (nT)
- Orientation translations are applied before printing.
See docs/Data-Format.md for more detail.

## Running tests (CTest)
The `i2c-pololu` unit tests are integrated with CTest.

Enable/Build/Run:
```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target i2c-pololu-tests
(cd build && ctest --output-on-failure)
```
Run the test executable directly:
```
./build/i2c-pololu-tests
```

## Documentation
- Getting Started: docs/Getting-Started.md
- Configuration reference: docs/Configuration.md
- Orientation and axes: docs/Orientation-and-Axes.md
- Data format: docs/Data-Format.md
- Hardware Setup (wiring, udev rules): docs/Hardware-Setup.md
- Troubleshooting: docs/Troubleshooting.md
- Development guide: docs/Development.md
- Contribution guidelines: CONTRIBUTING.md

## Command‑line help
```
./mag-usb -h
```
You should see something similar to:
```
Parameters:

   -B <reg mask>          :  Do built in self test (BIST).         [ Not implemented ]
   -C                     :  Read back cycle count registers before sampling.
   -c <count>             :  Set cycle counts as integer.          [ default: 200 decimal]
   -D <rate>              :  Set magnetometer sample rate.         [ TMRC reg 96 hex default ].
   -g <mode>              :  Device sampling mode.                 [ POLL=0 (default), CONTINUOUS=1 ]
   -O                     :  Path to Pololu port in /dev.          [ default: /dev/ttyACM0 ]
   -P                     :  Show all current settings and exit.
   -Q                     :  Verify presence of Pololu adaptor.
   -S                     :  List devices seen on i2c bus and exit.
   -T                     :  Verify Temperature sensor presence and version.
   -u                     :  Use named pipes for output.
   -i <path>              :  Path for input named pipe.
   -o <path>              :  Path for output named pipe.
   -V                     :  Display software version and exit.
   -h or -?               :  Display this help.
```

## Pololu adapter links
- https://www.pololu.com/product/5397
- https://www.pololu.com/product/5396


