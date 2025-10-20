# mag-usb
## Work in progress!
## DO NOT USE YET: to be updated further!

## Version 0.0.2

The utility **mag-usb** is a program intended to assist in testing the PNI RM3100 geomagnetic sensor.  
It is written in simple, portable C.

The rm3100 support boards were developed for use with the HamSCI project's Personal Space Weather Station [(PSWS)](https://hamsci.org/) TangerineSDR and Grape Space Weather monitors.  

These board pairs report magnetic field strength as three independent vectors, from which total field strength may be derived.  They also report the temperature in the immediate environment of the remotely placed sensor as a fraction of a degree C. 

They may also be used without other PSWS components for those interested in making magnetic field measurements for many purposes.  

Various pieces of software have been used to develop, test, and run these boards as part of the hardware suite or as standalone low-cost monitors of the Earth's magnetic field.

* The **mag-usb** utility is written as a Linux command line program and currently takes all configuration parameters from its commandline.

* This software **MIGHT** build and run there.

* This version is not tested for use with a Raspberry Pi-like device (but it might work there). 

* This software is **_not_** currently written to run on Windows. 

* This version of the code does NOT use libraries like pigpio that speak directly to Pi 3/4 hardware. 

* The code is written to use the [Pololu Isolated USB-to-I²C Adapter with Isolated Power](https://www.pololu.com/product/5397) and [Pololu Isolated USB-to-I²C Adapter](https://www.pololu.com/product/5396) boards.

* These should work with any host device that supports a USB 2 bus. 

* For use with magnetometer boards currently sold by [TAPR](https://tapr.org/product/tangerine-sdr-magnetometer/). For this use, they may require the [Pololu Isolated USB-to-I²C Adapter with Isolated Power](https://www.pololu.com/product/5397) that can supply 5v to the board.
 
The current pre-release code is 0.0.2

## To build:

Create a working directory and do:

    git clone https://github.com/wittend/mag-usb.git

Then do:

    $ cd mag-usb 
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

```
dave@big_server: ~/projects/mag-usb $ ./mag-usb -P /dev/ttyACM0
```

## For usage info type:
```
dave@big_server: ./mag-usb -h
```

You **MAY** see something like this:
```
Parameters:

   -B <reg mask>          :  Do built in self test (BIST).         [ Not implemented ]
   -C                     :  Read back cycle count registers before sampling.
   -c <count>             :  Set cycle counts as integer.          [ default: 200 decimal]
   -D <rate>              :  Set magnetometer sample rate.         [ TMRC reg 96 hex default ].
   -g <mode>              :  Device sampling mode.                 [ POLL=0 (default), CONTINUOUS=1 ]
   -P                     :  Path to Pololu port in /dev.          [ default: /dev/ttyACM0 ]
   -Q                     :  Verify presence of Pololu adaptor.
   -S                     :  List devices seen on i2c bus and exit.
   -T                     :  Verify Temperature sensor presence and version.
   -V                     :  Display software version and exit.
   -h or -?               :  Display this help.

```
 
## For more info on the Pololu USB to I2C Isolated adapter boards see:
https://www.pololu.com/product/5397
https://www.pololu.com/product/5396


