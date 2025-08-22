//=========================================================================
// interface.h
// 
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
// 
// Author:      David Witten, KD0EAG
// Date:        May 27,2025
// License:     GPL 3.0
// Note:        
//              
//              
//=========================================================================
#ifndef SWXCOMMANDS_h
#define SWXCOMMANDS_h
#include "../main.h"
//#include "main.h"

#define USE_I2C          TRUE
#define USE_POLOLU       1
#define USE_RGPIO        0
#define USE_LGPIO        0
#define USE_PIGPIO       0
#define USE_PIGPIO_IF2   0
#define USE_I2CFILES     0
#define UES_LINUXGPIO    0
#define USE_SPI          0
#define PI_INPUT         0
#define PI_OUTPUT        0

#if(USE_I2C)
    #if(USE_PIGPIO)
        #include <pigpio.h>
    #elif(USE_PIGPIO_IF2)
        #include <pigpiod_if2.h>
        int  init_IO();
        void terminate_IO(volatile ctlList *p);
        void verifySensor(volatile ctlList *p);
        int  readSensor(volatile ctlList *ctl);
        char *formatOutput(volatile ctlList *, char *outBuf);
        int  waitForRequest(volatile ctlList *);
    #elif(USE_RGPIO)
        #include <rgpio.h>
    #elif(USE_POLOLU)
        #include "i2c-pololu.h"
    #elif(USE_I2CFILES)
        #warning "Using Linux FILE functionality for I2C/GPIO."
    #elif
        #warning "Using I2C, but NO I2C/GPIO library defined!"
    #else
        #warning No I2C defined, Must be using SPI or something.
    #endif
#elif(USE_SPI)
    #warning"Lets not go ther right now."
#endif

#endif //SWXCOMMANDS_h

