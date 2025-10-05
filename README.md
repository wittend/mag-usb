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

* This This version is not intended or tested for use with a Raspberry Pi (but it might work there).

* This version of the code does NOT use libraries like pigpio that speak directly to Pi 3/4 hardware. 

* This software does use a Pololu i2c adapter board to communicate with the sensor.  The code is written to use the Pololu USB to I2C Isolated adapter boards and therefore should work with any device that supports a USB2 bus.  
 
The current pre-release code is 0.0.2

Just create a working directory and do:

    git clone https://github.com/wittend/mag-usb.git

Then do:

    $ cd mag-usb 
    $ make

```
dave@big_server: ~/projects/rm3100-runMag $ ./mag-usb -P /dev/ttyACM0
```

