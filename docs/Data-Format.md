# Data Format

mag-usb prints one JSON object per line to stdout. Each line represents a single sample of the magnetometer vector (and optionally other values in the future).

## Baseline schema
```
{ "ts": "DD Mon YYYY HH:MM:SS", "x": <float>, "y": <float>, "z": <float> }
```
- `ts` (string): UTC timestamp formatted like `25 Oct 2025 14:02:33` (RFC‑2822‑like time portion without timezone offset).
- `x`, `y`, `z` (number): Field components in nanoTesla (nT), with 3 decimal places printed.

Example:
```
{ "ts":"26 Oct 2025 14:20:00", "x":12345.678, "y":-234.500, "z":987.001 }
```

## Units and scaling
- Raw RM3100 counts are converted using configured gains and `NOS` (number‑of‑samples) register value.
- Outputs are provided in nanoTesla (nT). Internally the computation converts microTesla to nanoTesla by multiplying by 1000.

## Orientation translations
If configured, 90° increment rotations are applied to `(x,y,z)` before printing. See `docs/Orientation-and-Axes.md` and `[mag_orientation]` in `docs/Configuration.md`.

## Precision and rounding
- Values are printed with `%.3f` (three digits after decimal). This does not imply instrument accuracy; it is a display choice.

## Sampling cadence
- In POLL mode, each call to `formatOutput()` prints one sample.
- In CMM mode (continuous), the sample rate is controlled by `cmm_sample_rate`; see `docs/Configuration.md`.

## Errors and diagnostics output
- Informational and error messages (e.g., adapter checks) are printed to stdout/stderr around the JSON lines. If you need a clean stream of JSON only, redirect stderr and/or prefilter lines not starting with `{`.

## Logging and pipes
- If you enable logging or named pipes in the configuration, the same JSON lines can be written to files or pipes. See `docs/Configuration.md` (output section).
