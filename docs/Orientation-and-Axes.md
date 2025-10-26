# Orientation and Axes

This document explains how mag-usb handles magnetometer orientation and how to configure 90° rotations around each axis.

## Axis conventions
- The RM3100 outputs three orthogonal components X, Y, Z.
- mag-usb treats these axes as a right‑handed coordinate system.
- Positive rotations follow the right‑hand rule (thumb points along the axis of rotation; curl of fingers is direction of positive rotation).

## Why orientation adjustments?
Depending on how the sensor PCB is mounted in your enclosure, its axes may not align with your desired reference frame. mag-usb can rotate the measured vector by multiples of 90° around the X, Y, and Z axes to align with your installation.

## Configuration
Set the following keys in `config.toml` under `[mag_orientation]`:
```
[mag_orientation]
mag_translate_x = <int>   # -180, -90, 0, 90, 180
mag_translate_y = <int>   # -180, -90, 0, 90, 180
mag_translate_z = <int>   # -180, -90, 0, 90, 180
```
Rules:
- Allowed values: `-180, -90, 0, 90, 180`. Any other value is treated as `0`.
- `180` and `-180` are treated the same (flip both perpendicular axes).
- Rotations are applied in this order: rotate around X, then around Y, then around Z.
- The rotations affect the other two components:
  - Rotate around X → affects Y and Z.
  - Rotate around Y → affects X and Z.
  - Rotate around Z → affects X and Y.

## Examples
Let the input vector be `(X, Y, Z)`.

- Rotate +90° about X:
  - `(X, Y, Z)` → `(X, -Z, Y)`
- Rotate -90° about X:
  - `(X, Y, Z)` → `(X, Z, -Y)`
- Rotate 180° about X:
  - `(X, Y, Z)` → `(X, -Y, -Z)`

- Rotate +90° about Y:
  - `(X, Y, Z)` → `(Z, Y, -X)`
- Rotate -90° about Y:
  - `(X, Y, Z)` → `(-Z, Y, X)`
- Rotate 180° about Y:
  - `(X, Y, Z)` → `(-X, Y, -Z)`

- Rotate +90° about Z:
  - `(X, Y, Z)` → `(-Y, X, Z)`
- Rotate -90° about Z:
  - `(X, Y, Z)` → `(Y, -X, Z)`
- Rotate 180° about Z:
  - `(X, Y, Z)` → `(-X, -Y, Z)`

Compound rotations apply these in order X → Y → Z. For example, X=+90°, Z=-90°:
1) X=+90°: `(X, Y, Z)` → `(X, -Z, Y)`
2) Z=-90°: `(X, -Z, Y)` → `(-(-Z), -X, Y)` → `(Z, -X, Y)`

## Verification
After editing `config.toml`, run:
```
./mag-usb -P
```
You should see:
```
Orientation translate (deg XYZ): <x>, <y>, <z>
```
Then start normal sampling and confirm that reported axes behave as expected when you rotate the sensor in known directions.
