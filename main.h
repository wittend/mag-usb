//=========================================================================
// main.h
//
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// License:     GPL 3.0
// Note:        replaces i2c.c (using file system calls to read(), write(), etc.
//              with calls to pigpio. 
//              Also adding callbacks on GPIO27 for PPS rising edge.
//=========================================================================
#ifndef SWX3100MAIN_h
#define SWX3100MAIN_h

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
// #include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
// #include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
// #include <fcntl.h>
// #include <memory.h>
#include "src/i2c_pololu.h"

// #include "src/interface.h"      // defines the I2C interface to be used.
#include "src/i2c.h"
#include "src/rm3100.h"
#include "src/MCP9808.h"

//------------------------------------------
// Debugging output
//------------------------------------------
#define _DEBUG          FALSE
#define CONSOLE_OUTPUT  TRUE

//------------------------------------------
// Pi related stuff
//------------------------------------------
#define USE_WAITFOREDGE FALSE
#define FOR_GRAPE2      FALSE
#define PPS_TIMEOUTSECS 2.0

//------------------------------------------
// Macros and runtime options.
//------------------------------------------
#define OUTPUT_PRINT    stdout
#define OUTPUT_ERROR    stderr

#define USE_PIPES           FALSE
#define MAGDATA_VERSION     "0.2.0"
#define UTCBUFLEN           64
#define MAXPATHBUFLEN       1025
#define XYZ_BUFLEN          9

//------------------------------------------
// Control Parameter List struct
//------------------------------------------
/**
 * @struct ctlList
 * @brief Represents a control list for managing various I2C devices and system parameters.
 *
 * This structure is designed to store information related to various system configuration,
 * device handles, and control parameters needed for I2C-based device communication and processing.
 *
 * The ctlList struct includes members for maintaining I2C bus configuration, addresses for
 * connected devices, handles for various modules, and parameters for system settings and
 * operational modes.
 *
 * @typedef tag_ctlList ctlList
 *
 * @var ctlList::adapter
 * pololu_i2c_adapter instance that handles I2C communication.

 * @var ctlList::ppsHandle
 * Handle for pulse-per-second (PPS) signal management.

 * @var ctlList::edge_cb_id
 * Identifier for edge callback management.

 * @var ctlList::magHandle
 * Handle for the magnetometer device.

 * @var ctlList::localTempHandle
 * Handle for the local temperature sensor.

 * @var ctlList::remoteTempHandle
 * Handle for the remote temperature sensor.

 * @var ctlList::magnetometerAddr
 * Address of the magnetometer device on the I2C bus.

 * @var ctlList::localTempAddr
 * Address of the local temperature sensor on the I2C bus.

 * @var ctlList::remoteTempAddr
 * Address of the remote temperature sensor on the I2C bus.

 * @var ctlList::doBistMask
 * Mask value for Built-In Self Test (BIST) configuration.

 * @var ctlList::cc_x
 * Coefficient for 'x' component.

 * @var ctlList::cc_y
 * Coefficient for 'y' component.

 * @var ctlList::cc_z
 * Coefficient for 'z' component.

 * @var ctlList::x_gain
 * Gain factor for the x-axis.

 * @var ctlList::y_gain
 * Gain factor for the y-axis.

 * @var ctlList::z_gain
 * Gain factor for the z-axis.

 * @var ctlList::XYZ
 * Array for storing XYZ components, size is 9.

 * @var ctlList::TMRCRate
 * Timer sampling rate.

 * @var ctlList::CMMSampleRate
 * Common Mode Sample Rate.

 * @var ctlList::samplingMode
 * Flag indicating the mode of sampling for sensors.

 * @var ctlList::NOSRegValue
 * Value for the NOS register.

 * @var ctlList::DRDYdelay
 * Delay for Data Ready (DRDY) signal.

 * @var ctlList::readBackCCRegs
 * Flag indicating whether to read back the CC registers.

 * @var ctlList::magRevId
 * Revision ID of the magnetometer device.

 * @var ctlList::i2cBusNumber
 * Identifier for the I2C bus number being utilized.

 * @var ctlList::tsMilliseconds
 * Timestamp value in milliseconds.

 * @var ctlList::Version
 * Pointer to a string representing the version information.

 * @var ctlList::usePipes
 * Flag indicating whether pipes are used.

 * @var ctlList::pipeInPath
 * Path to the input pipe file.

 * @var ctlList::pipeOutPath
 * Path to the output pipe file.
 */
typedef struct tag_ctlList
{
    int fd;
    pololu_i2c_adapter adapter;

    int ppsHandle;
    unsigned edge_cb_id;
    
    unsigned magHandle;
    unsigned localTempHandle;
    unsigned remoteTempHandle;

    int  magnetometerAddr;
    int  localTempAddr;
    int  remoteTempAddr;

    int  doBistMask;

    int  cc_x;
    int  cc_y;
    int  cc_z;

    int  x_gain;
    int  y_gain;
    int  z_gain;

    int32_t XYZ[9];

    int  TMRCRate;
    int  CMMSampleRate;

    int  samplingMode;

    int  NOSRegValue;

    int  DRDYdelay;

    int  readBackCCRegs;
    uint8_t magRevId;

    int  i2cBusNumber;

    int  tsMilliseconds;
    char *Version;
    int  usePipes;
    char *pipeInPath;
    char *pipeOutPath;
} ctlList;


//------------------------------------------
// Prototypes
//------------------------------------------
int  main(int argc, char** argv);
void onEdge(void);
char *formatOutput(volatile ctlList *p, char *outBuf);
int  initGPIO(volatile ctlList *p);
void termGPIO(volatile ctlList *p);
int  verifyMagSensor(volatile ctlList *p);
int  initMagSensor(volatile ctlList *p);
int  initTempSensors(volatile ctlList *p);

int  readLocalTemp(volatile ctlList *p);
int  readRemoteTemp(volatile ctlList *p);
//int  readMagCMM(volatile ctlList *p);
int  readMagPOLL(volatile ctlList *p);

void showErrorMsg(int temp);

//void showPIGPIOErrMsg(int rv);

#endif //SWX3100MAIN_h

