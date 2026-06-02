## Second Claude Code Review for mag-usb. 
Focused on the scaling and configuration issues that have been observed in live data.

1. [**Note**: This is a follow-up to the first Claude code review, which focused on code quality and maintainability. This review focuses on correctness and operator experience.]
2. [**Note**: A second review using the Gemini model was performed to examine standalone code quality and maintainability, without the context of the live data issues. That review is documented separately.]
3. [**Note**: This represents feedback within a team consisting of three humans: Mike, Rob, and Dave.  Dave is the author of the code under review, and Rob is a third team member working with Mike. Claude, Gemini, and their friends are AI Assistants.  Claude is providing objective feedback. Dave is also being assisted by one of more AI tools.]
4. [**Note**: Mike, Rob, and Claude are working together to integrate this program (mag-usb) into a larger system (sigmond), that may also incorporate other components (mag-recorder, magrec, a global installation process, etc.) when used with sigmond.]
5. [**Note**: mag-usb must also retain the ability to be configured independently.]
6. [**Note**: The tone is collaborative and constructive, with a focus on improving the code and the operator experience.]

### PR-1: drop the spurious /NOSRegValue factor in POLL-mode scaling

src/main.c:606-608:
* xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain) * 1000;
* The PNI formula Dave himself pasted into src/magdata.c:142-148 is µT = raw / (0.3671·CC + 1.5), no NOS term. With NOSRegValue defaulting to 60, every reported value is off by ~60× — that's exactly what live data on my desk shows (-210..+412
where Earth-field axis components should be 10,000–30,000 nT). NOS only belongs in the scaling for CMM-with-averaging, where the chip has been programmed to sum N samples internally; in single-shot POLL mode the chip already returns per-sample values. 
* Suggested fix: drop the /NOSRegValue. If Dave wants to keep an averaging path for a future CMM mode, gate it on p->samplingMode == CMM and only when the chip's NOS register has actually been written.

### PR-2: actually program the chip's CC and NOS registers (or remove the knobs)

src/magdata.c:166-180 (setCycleCountRegs) 
* has every i2c_write commented out. 
* Same in setNOSReg at magdata.c:106-114: 
    * the whole body is dead. Net effect: cc_x/y/z and NOSRegValue in config are host-side ghosts. The chip stays at power-on
defaults (CC=200/axis), and any operator who edits cc_x = 400 in tools/config.toml thinks they've doubled their sensitivity but actually they've just made the host divide by a bigger number. This is upstream of the magnitude bug in PR-1 — both
fixes are needed.

Chosen approach:
* Restore the writes using the Pololu I²C path (the rest of the I/O in this binary already does this), so config takes effect. Cleaner long-term.
* **Make note** in _documentation_ and _comment in code_: "[**Path not taken**]: remove the host-side CC/NOS knobs entirely, hard-code gain from chip defaults, and document that. Smaller patch, less flexible."

Either is fine; the current limbo is what's broken.

### PR-3: fix the example tools/config.toml

tools/config.toml ships cc_x = cc_y = cc_z = 400
* This combined with PR-2 silently mis-scales. After PR-2 lands this becomes a correctness fix or a delete-the-knob change, depending on which path Dave picks.

* mag-usb layering PRs (so mag-recorder can fully own operator config)

* The pattern I'd like to align with: mag-recorder is the program the operator configures; mag-usb is the sensor driver mag-recorder drives. Operator never sees /etc/mag-usb/anything. All knobs live in /etc/mag-recorder/mag-recorder-config.toml.

### PR-4: accept config-file path on the CLI

src/main.c currently auto-discovers /etc/mag-usb/config.toml then ./config.toml, with no way to point it elsewhere. 
* Make it accept -f <path> (or --config <path> if Dave wants to move to getopt_long) — note -c is already taken by cycle-count.
* When -f is given, skip the auto-discovery entirely so the binary's behavior is fully determined by argv. 
    - mag-recorder will then render a private TOML at /etc/mag-recorder/mag-usb-driver.toml and pass it explicitly.

Optional sweetener: also accept -A <hex> (single-knob address override) for the common case of "everything else default, just point at 0x23." Saves a config file in the hand-debug workflow.

### PR-5: make -P (show settings) print chip state, not host state

Today -P prints the host's intended values. After PR-2 those should match the chip, but a real -P that reads back the CC/NOS/TMRC registers would catch any future regression where the writes silently stop landing. Small but high-leverage.

What this lets mag-recorder do (no Dave work, just queued on our side)

1. Render /etc/mag-recorder/mag-usb-driver.toml as an install step — sourced from [mag] keys in mag-recorder-config.toml. Operator never edits the driver TOML.
2. Patch supervisor.py:_mag_usb_source to construct argv = [binary, "-O", device, "-f", "/etc/mag-recorder/mag-usb-driver.toml"]. The currently-dangling mag_usb_config key in mag-recorder-config.toml finally connects to something.
3. Add an install step that drops install/99-PololuI2C.rules from upstream mag-usb into /etc/udev/rules.d/ and runs udevadm trigger. This is the "predictable device regardless of USB port" piece — /dev/ttyMAG0 becomes the stable symlink
   mag-recorder's config already points at by default.
4. Add magrec to dialout in deploy (already done in the service file's SupplementaryGroups=dialout; confirm the user-creation step also sets it).
5. Strengthen mag-recorder validate to fail loudly when the configured [mag].i2c_address doesn't match what mag-usb -M reports — so an operator with a different carrier-board strapping sees a clear
    validate error instead of a silent NACK at runtime.
Once PR-1+2+4 land in wittend/mag-usb (and we cherry-pick / merge into our sigmond-integration branch):

### PR-6: render /etc/mag-recorder/mag-usb-driver.toml from mag-recorder-config.toml

1. Render /etc/mag-recorder/mag-usb-driver.toml as an install step — sourced from [mag] keys in mag-recorder-config.toml. Operator never edits the driver TOML.
2. Patch supervisor.py:_mag_usb_source to construct argv = [binary, "-O", device, "-f", "/etc/mag-recorder/mag-usb-driver.toml"]. The currently-dangling mag_usb_config key in mag-recorder-config.toml finally connects to something.
3. Add an install step that drops install/99-PololuI2C.rules from upstream mag-usb into /etc/udev/rules.d/ and runs udevadm trigger. This is the "predictable device regardless of USB port" piece — /dev/ttyMAG0 becomes the stable symlink
   mag-recorder's config already points at by default.
4. Add magrec to dialout in deploy (already done in the service file's SupplementaryGroups=dialout; confirm the user-creation step also sets it).
5. Strengthen mag-recorder validate to fail loudly when the configured [mag].i2c_address doesn't match what mag-usb -M reports — so an operator with a different carrier-board strapping sees a clear validate error instead of a silent NACK at
   runtime.
6. Drop /etc/mag-usb/ from documentation entirely — README/PROVENANCE shouldn't mention it once PR-4 lands.

## Suggested order for your conversation with Dave

1. PR-1 (60× scaling — operator-visible correctness; this is the headline)
2. PR-2 (CC/NOS writes — explains why the scaling looks the way it does, and unblocks per-axis tuning)
3. PR-3 (example TOML — trivial follow-on)
4. PR-4 (config-file CLI flag — layering; this is the one that lets mag-recorder act like a proper sigmond client)
5. PR-5 (-P reads chip state — diagnostic hygiene)


