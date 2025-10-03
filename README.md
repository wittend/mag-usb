# mag-usb
## Work in progress!
## DO NOT USE YET: to be updated further!

## Version 0.0.1

## Version 0.0.1

The rm3100 support boards were developed for use with the Personal Space Weather Station (PSWS) TangerineSDR and Grape Space Weather monitors.  These board pairs report magnetic field strength as three independent vectors, from which a total field strength may be derived.  They also report the temperature in the immediate environment of the remotely placed sensor and at the near end of the pair as a fraction of a degree C.  They may also be used standalone with only a Pi or Pi clone board.  Various pieces of software have been used to develop, test, and run these boards as part of the hardware suite or as standalone low-cost monitors of the Earth's magnetic field.

Currently the program code most used in this project is called **mag-usb**. 

The utility **mag-usb** is a program intended to assist in testing the PNI RM3100 geomagnetic sensor.  
It is written in simple, portable C.

* The **mag-usb** utility is written as a Linux command line program and takes all configuration parameters from its commandline. 

* This requires the Pi I2C kernel driver is activated (usually by configuring I2C I/O in the raspi-setup utility).

* This version of the code is dependent on the presence of the pigpio library to build.

* This software was written to be the Raspberry Pi 3/4 only, and will not work on similar boards (including the PI-5). It has been tested, if not thoroughly. 
 
The current pre-release code is 0.0.1

Just clone this project into your home directory on the Raspberry Pi or board with similar bus using:

    git clone https://github.com/wittend/mag-usb.git

Then do:

    $ cd mag-usb 
    $ make


## Example on Raspberry Pi 3/4, using logging:
 
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

```
dave@raspi-3: ~/projects/rm3100-runMag $ ./mag-usb -P /dev/ttyACM0
```

