## USB Device in Proxmox Container

To make a USB device visible in `/dev` within a Proxmox LXC container running Debian Trixie, you need to pass through the device using the container's configuration. The most reliable method involves using persistent device paths like `/dev/serial/by-id` or directly passing the device via its major and minor numbers.

First, identify the USB device on the Proxmox host using `lsusb` to find the vendor and product IDs, and then use `ls -l /dev/serial/by-id` to find the stable symlink path (e.g., `usb-FTDI_FT230X_Basic_UART_D307ORET-if00-port0`) which links to a device like `/dev/ttyUSB0`  This path remains consistent even after unplugging and replugging the device.

In the Proxmox web interface, navigate to the container's Resources, click Add, then Device Passthrough, and enter the full path such as `/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D307ORET-if00-port0`  This method is considered clean and stable 

Alternatively, if you need more control or the persistent path is unavailable, edit the container's configuration file located at `/etc/pve/lxc/<container_id>.conf`. Use `lsusb` to find the bus and device numbers (e.g., 001, 003), then run `ls -al /dev/bus/usb/001/003` to get the major and minor numbers (e.g., 189, 130)  Add the following lines to the config file:

```
lxc.cgroup2.devices.allow: c 189:* rwm
lxc.mount.entry: /dev/ttyUSB0 dev/ttyUSB0 none bind,optional,create=file
```

Replace `189` with your device’s major number and `ttyUSB0` with the correct device name  The `rwm` flag grants read, write, and mknod permissions  After saving, restart the container.

If the device appears but has incorrect ownership (e.g., `nobody:nogroup`), fix it by changing the device’s owner and group on the host using the container’s user and group IDs. For example, if the container’s dialout group ID is 20, run `chown 100000:100020 /dev/ttyUSB0` on the host  This ensures the device is accessible within the container.

Finally, verify the device is visible inside the container by running `ls /dev` or `lsusb` after the container restarts 

## Elaborated
To ensure a USB device appears reliably in `/dev` within a Debian Trixie LXC container on Proxmox and survives reboots or replugging, follow these steps:

### 1. **Use Persistent Device Paths**
Instead of relying on transient paths like `/dev/ttyUSB0`, use stable symlinks from `/dev/serial/by-id/` or create custom UDEV rules.

- On the host, run:
  ```bash
  ls -l /dev/serial/by-id/
  ```
  This gives a persistent path like `usb-Silicon_Labs_CP210x-if00-port0` → `ttyUSB0`.

- In the container config (`/etc/pve/lxc/<ID>.conf`), add:
  ```
  lxc.mount.entry: /dev/serial/by-id/usb-Silicon_Labs_CP210x-if00-port0 dev/ttyUSB0 none bind,optional,create=file
  ```
  This ensures the device is consistently mapped even after disconnects.

### 2. **Grant Device Permissions**
Allow the container to access the device by its major number.

- Find the major number:
  ```bash
  ls -l /dev/ttyUSB0
  # Output: crw-rw---- 1 root dialout 188, 0 Apr  5 10:00 /dev/ttyUSB0
  ```
  Here, `188` is the major number.

- Add to the container config:
  ```
  lxc.cgroup2.devices.allow: c 188:* rwm
  ```

### 3. **Preserve Permissions Across Reboots**
If permissions reset after reboot, use systemd or tmpfiles.

- Create a tmpfile rule:
  ```bash
  echo 'z /dev/ttyUSB0 660 100000 100000' > /etc/tmpfiles.d/usb.conf
  ```
  Replace `100000` with your container’s UID/GID if unprivileged.

- Or use a systemd service:
  ```ini
  # /etc/systemd/system/usb_owner.service
  [Unit]
  Description=Set USB ownership
  [Service]
  Type=oneshot
  ExecStart=chown 100000:100000 /dev/ttyUSB0
  [Install]
  WantedBy=multi-user.target
  ```
  Enable with:
  ```bash
  systemctl daemon-reload
  systemctl enable usb_owner.service
  ```

### 4. **Optional: Create Custom UDEV Rules**
For full control, create a UDEV rule on the host.

- Example rule (`/etc/udev/rules.d/99-usb-serial.rules`):
  ```
  SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", SYMLINK+="mydevice", GROUP="dialout", MODE="0660"
  ```
- Reload rules:
  ```bash
  udevadm control --reload && udevadm trigger
  ```

Then pass `/dev/mydevice` into the container.

