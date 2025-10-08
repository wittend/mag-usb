## Beelink or other device configuration notes

### Creating a udev rules file for the Pololu adapter.
The idea here is to create a predicable entry in /dev for the software to connect to.

The device entry name used is /dev/ttyMAG0.  This is used bu the software as the default device entry path to open.

This default may be overridden from the command line using a parameter specification like ' _-P /dev/ttyACMx_ ' if needed.

##### From: 99-PololuI2C.rules
```
# Attached to USB-A connector on front of LattePanda Sigma (LP-Sig0)
#-------------------------------------------------------------------------
# Pololu dodumentation for installing on Windows indicates that the VID/Pid pairs
# are:
#     USB\VID_1FFB&PID_2502  (for the 'Pololu Isolated USB-to-I2C Adapter')
#     USB\VID_1FFB&PID_2503  (Pololu Isolated USB-to-I2C Adapter with Isolated Power') (I think)
#
# $ lsusb
# ...
# 'Pololu Corporation Pololu Isolated USB-to-I2C Adapter with Isolated Power'
# Bus 003 Device 023: ID 1ffb:2503
# ...
#
# $ lsusb -t
# ...
# /:  Bus 003.Port 001: Dev 001, Class=root_hub, Driver=xhci_hcd/12p, 480M
#    |__ ...
#    |__ ...
#    |__ Port 006: Dev 023, If 0, Class=Communications, Driver=cdc_acm, 12M
#    |__ Port 006: Dev 023, If 1, Class=CDC Data, Driver=cdc_acm, 12M
#    |__ ...
# ...
#
# udevadm info --name=/dev/ttyACM2 --attribute-walk
#  looking at device '/devices/pci0000:00/0000:00:14.0/usb3/3-7/3-7.1/3-7.1.2/3-7.1.2:1.0/tty/ttyACM2':
#    KERNEL=="ttyACM2"
#    SUBSYSTEM=="tty"
#    DRIVER==""
#    ATTR{power/async}=="disabled"
#  ...
#
#  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb3/3-7/3-7.1/3-7.1.2/3-7.1.2:1.0':
#    KERNELS=="3-7.1.2:1.0"
#    SUBSYSTEMS=="usb"
#    DRIVERS=="cdc_acm"
#  ...
# ...
# 
# Reload rules:
# 		$ sudo udevadm control --reload-rules && sudo udevadm trigger
# Test:
# 		$ ls .l /dev/ttyMAG0
#
KERNEL=="ttyACM[0-9]*", SUBSYSTEM=="tty", ATTRS{idVendor}=="1ffb", ATTRS{idProduct}=="2503", SYMLINK+="ttyMAG0"
```

```
$ udevadm info --name=/dev/ttyACM2 --attribute-walk

Udevadm info starts with the device specified by the devpath and then
walks up the chain of parent devices. It prints for every device
found, all possible attributes in the udev rules key format.
A rule to match, can be composed by the attributes of the device
and the attributes from one single parent device.

  looking at device '/devices/pci0000:00/0000:00:14.0/usb3/3-6/3-6:1.0/tty/ttyACM2':
    KERNEL=="ttyACM2"
    SUBSYSTEM=="tty"
    DRIVER==""
    ATTR{power/async}=="disabled"
    ATTR{power/control}=="auto"
    ATTR{power/runtime_active_kids}=="0"
    ATTR{power/runtime_active_time}=="0"
    ATTR{power/runtime_enabled}=="disabled"
    ATTR{power/runtime_status}=="unsupported"
    ATTR{power/runtime_suspended_time}=="0"
    ATTR{power/runtime_usage}=="0"

  looking at parent device '/devices/pci0000:00/0000:00:14.0/usb3/3-6/3-6:1.0':
    KERNELS=="3-6:1.0"
    SUBSYSTEMS=="usb"
    DRIVERS=="cdc_acm"
    ATTRS{authorized}=="1"
    ATTRS{bAlternateSetting}==" 0"
    ATTRS{bInterfaceClass}=="02"
    ATTRS{bInterfaceNumber}=="00"
    ATTRS{bInterfaceProtocol}=="00"
    ATTRS{bInterfaceSubClass}=="02"
    ATTRS{bNumEndpoints}=="01"
    ATTRS{bmCapabilities}=="6"
    ATTRS{iad_bFirstInterface}=="00"
    ATTRS{iad_bFunctionClass}=="02"
    ATTRS{iad_bFunctionProtocol}=="00"
    ATTRS{iad_bFunctionSubClass}=="02"
    ATTRS{iad_bInterfaceCount}=="02"
    ATTRS{interface}=="Pololu Isolated USB-to-I2C Adapter with Isolated Power"
    ATTRS{physical_location/dock}=="no"
    ATTRS{physical_location/horizontal_position}=="left"
    ATTRS{physical_location/lid}=="no"
    ATTRS{physical_location/panel}=="top"
    ATTRS{physical_location/vertical_position}=="upper"
    ATTRS{power/async}=="enabled"
    ATTRS{power/runtime_active_kids}=="0"
    ATTRS{power/runtime_enabled}=="enabled"
    ATTRS{power/runtime_status}=="suspended"
    ATTRS{power/runtime_usage}=="0"
    ATTRS{supports_autosuspend}=="1"

```