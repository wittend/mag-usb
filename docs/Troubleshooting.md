# Troubleshooting

Common issues and how to resolve them.

## Adapter not found or permission denied
- Symptom: mag-usb -Q reports errors opening /dev/ttyACM0, or EACCES.
- Fixes:
  - Ensure your user is in the dialout (or equivalent) group: `sudo usermod -aG dialout $USER` then re-login.
  - Install udev rule: see docs/Hardware-Setup.md and install/99-PololuI2C.rules.
  - Verify the device node exists: `ls -l /dev/ttyACM*`.
  - Check dmesg: `dmesg | tail -n 50`.

## Wrong device path (ACM1 vs ACM0)
- The ttyACM number can change. Either probe available devices or create a persistent symlink via udev.

## Build fails
- Ensure CMake 3.22+ and a recent GCC/Clang.
- Try a clean build directory.
- If errors are warnings-as-errors, you can temporarily disable `-DENABLE_WERROR=ON`.

## Tests fail
- Some tests interact with the i2c-pololu logic; ensure the adapter is not locked by another process.
- Re-run with verbose CTest: `(cd build && ctest -VV)`.

## No data from sensor
- Check wiring (SDA/SCL/GND, optional 5V).
- Confirm pull-up resistors present.
- Verify device addresses match expected values in config (MCP9808/Magnetometer addresses).

## Getting more help
- Open an issue with logs:
  - Your distro and kernel version
  - Output of `lsusb`, `dmesg` after plugging the adapter
  - mag-usb CLI, version, and full console output
