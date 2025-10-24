# Hardware Setup

This guide helps you connect the Pololu USB-to-I²C adapter to the RM3100-based sensor and prepare your Linux system.

## Hardware
- Pololu Isolated USB-to-I²C Adapter:
  - 5396 (no power isolation) or 5397 (with isolated power output)
- RM3100 magnetometer board (e.g., TAPR/TangerineSDR magnetometer)
- Appropriate wiring for SDA, SCL, GND, and optional +5V

## Wiring
- Connect:
  - SDA ↔ SDA
  - SCL ↔ SCL
  - GND ↔ GND
  - +5V (optional, only if your board requires power from adapter 5397)
- Confirm pull-ups are present on SDA/SCL (many boards include them).

## Linux device path
- The adapter appears as a ttyACM device (e.g., /dev/ttyACM0).
- Check dmesg or lsusb for details; examples are in assets/.

## Stable permissions and naming
- A sample udev rule is provided at install/99-PololuI2C.rules. To install:
```
sudo cp install/99-PololuI2C.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```
- After replugging the adapter, confirm permissions and any symlink names defined in your rule.

## Verifying connectivity
- Use mag-usb's quick check flag:
```
./mag-usb -P /dev/ttyACM0 -Q
```
- If you see permission errors, check your udev setup and group membership (dialout or equivalent on your distro), or run temporarily with sudo (not recommended long-term).
