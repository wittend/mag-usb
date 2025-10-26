# Getting Started

This guide helps you install, build, and run mag-usb on a typical Linux system.

## Prerequisites
- Linux host with USB 2.0 support
- CMake 3.22+
- A C compiler (GCC or Clang)
- Optional: JetBrains CLion for an IDE workflow
- Pololu Isolated USB-to-I²C Adapter (products 5396 or 5397)
- RM3100-based magnetometer board

## Hardware connection
1. Connect the Pololu USB-to-I²C adapter to your host via USB.
2. Wire SDA/SCL/GND (and 5V if required) between the adapter and the sensor board.
3. On Linux, the adapter typically appears as /dev/ttyACM0 (or ACM1, etc.).

For details and udev rules to stabilize the device path, see docs/Hardware-Setup.md.

## Build
You can build using CLion or plain CMake.

CLion:
- Open the project folder.
- Select the Debug or Release profile.
- Build the target: mag-usb

Command line (Release example):
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target mag-usb
```

## Quick run
```
./build/mag-usb -O /dev/ttyACM0 -Q
```
Flags:
- -O sets the Pololu device path.
- -Q checks adapter presence/availability.

Print current settings (and exit):
```
./build/mag-usb -P
```
If `config.toml` is present in the working directory, its values are shown; otherwise defaults are used.

Run with help:
```
./mag-usb -h
```

## Tests (optional)
Enable and run unit tests:
```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target i2c-pololu-tests
(cd build && ctest --output-on-failure)
```

## Next steps
- Review README.md (command-line help section) and docs/Configuration.md for available flags and configuration keys.
- If you use the adapter regularly, consider installing the provided udev rule (install/99-PololuI2C.rules) to get stable permissions and device naming.
