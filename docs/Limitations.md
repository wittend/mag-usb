# Limitations

This document summarizes the expected limitations of mag-usb in its current form (v0.0.3 pre‑release).

## Platform support
- Target platform: Linux.
- Windows: not supported.
- macOS: untested; may work with appropriate serial/tty driver.
- Raspberry Pi and SBCs: may work, but native GPIO/I²C libraries are not used in this build; the Pololu USB‑to‑I²C adapter is the intended path.

## Hardware assumptions
- Designed for the Pololu Isolated USB‑to‑I²C Adapter (5396/5397).
- Requires proper wiring, power, and pull‑ups as per your RM3100 carrier board.

## Sampling modes
- POLL mode is supported.
- CMM (continuous) mode fields exist in configuration; behavior may be limited by adapter throughput and host scheduling.

## Timing and throughput
- JSON printing and host scheduling can introduce jitter. If you need strict timing, consider capturing raw data with a dedicated process and minimal logging.

## Orientation granularity
- Only right‑handed rotations in 90° increments about X/Y/Z are supported via `[mag_orientation]`.
- Arbitrary angles and fine calibration (soft‑/hard‑iron compensation) are not implemented.

## Accuracy and units
- Reported values are in nanoTesla (nT). Display precision is 0.001 nT, which does not imply sensor accuracy.
- Absolute accuracy depends on sensor calibration, installation environment, and configuration (cycle counts, gains).

## Configuration loading
- The program looks for `config.toml` in the current working directory only. If missing or partially invalid, defaults are used silently (warnings may be printed).

## Logging and pipes
- File logging and named pipes are optional features; ensure correct permissions and paths. When disabled, related paths are ignored.

## Error handling
- Some runtime errors print messages interleaved with JSON lines. If you require a clean data stream, redirect  and/or prefilter non‑JSON lines.

## Backward compatibility
- Command‑line flags may evolve; use `-h` to discover current support. Configuration keys not recognized are ignored.
