## Force USB ttyACM to specific /dev device

Yes, there is a way in Linux to force a USB ttyACM type device to always attach to a specific device file in /dev by using udev rules to create persistent symbolic links This ensures that the device is consistently accessible under a predictable name, regardless of the order in which devices are connected or the system's boot sequence.

To achieve this, you need to create a custom udev rule file in the /etc/udev/rules.d/ directory, such as /etc/udev/rules.d/49-custom.rules The rule should identify the specific USB device using unique attributes like the vendor ID (`idVendor`) and product ID (`idProduct`) obtained from `lsusb` For example, to assign a U-Blox GNSS receiver to a symbolic link named `/dev/ttyGPS`, the rule would be:

```
KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{idVendor}=="1546", ATTRS{idProduct}=="01a8", SYMLINK+="ttyGPS"
```

This rule uses the `SYMLINK` directive to create a persistent symbolic link that points to the actual device file (e.g., `/dev/ttyACM0` or `/dev/ttyACM1`) After creating the rule, reload the udev rules with `udevadm control --reload-rules` and reboot the system to apply the changes Once configured, services or applications can reference the consistent symbolic name (e.g., `/dev/ttyGPS`) instead of relying on the potentially changing `/dev/ttyACM*` name

If multiple devices share the same `idVendor` and `idProduct`, you can use additional unique attributes such as the device's serial number (`ATTRS{serial}`) or its kernel path (`KERNELS`) to distinguish them For instance, using `KERNELS=="1-4:1.0"` can tie the rule to a specific USB port on a hub, ensuring consistent naming even when multiple devices are present

## Elaboration

To force a USB ttyACM device to always appear under a specific name in `/dev`, you can create a **persistent symbolic link** using **udev rules** based on unique device attributes.

### 1. Identify Unique Device Attributes
First, connect the device and use:
```bash
udevadm info --name=/dev/ttyACM0 --attribute-walk
```
Look for stable, unique identifiers such as:
- `idVendor` and `idProduct` (identifies device type)
- `serial` (unique per device)
- `KERNELS` (e.g., `1-4:1.0` — identifies physical USB port)

### 2. Create a udev Rule
Create a rule file:
```bash
sudo nano /etc/udev/rules.d/99-mydevice.rules
```

#### Example Rules:
- **By vendor/product (for single device of this type):**
  ```
  SUBSYSTEM=="tty", KERNEL=="ttyACM[0-9]*", ATTRS{idVendor}=="1546", ATTRS{idProduct}=="01a8", SYMLINK+="ttyGPS"
  ```

- **By serial number (for multiple identical devices):**
  ```
  SUBSYSTEM=="tty", KERNEL=="ttyACM[0-9]*", ATTRS{serial}=="ABC123", SYMLINK+="ttyGPS1"
  ```

- **By physical USB port (assigns name based on plug-in location):**
  ```
  SUBSYSTEM=="tty", KERNEL=="ttyACM[0-9]*", KERNELS=="1-4:1.0", SYMLINK+="ttyGPS"
  ```

### 3. Reload and Test
Apply the rules:
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```
Replug the device and verify:
```bash
ls -l /dev/ttyGPS
```

### Key Notes:
- Use `SYMLINK+="name"` to create a persistent alias like `/dev/ttyGPS`.
- For multiple similar devices, **use `serial` or `KERNELS`** to avoid conflicts.
- The rule file should end with `.rules` and ideally start with a high number (e.g., `99-`) to ensure it runs last.
- Ensure the file has a trailing newline and is owned by root.



