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
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>

// #include "src/interface.h"      // defines the I2C interface to be used.
// #include "i2c.h"
#include "i2c_pololu.h"
#include "rm3100.h"
#include "MCP9808.h"

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
 * @struct pList
 * @brief Represents a control list for managing various I2C devices and system parameters.
 *
 * This structure is designed to store information related to various system configuration,
 * device handles, and control parameters needed for I2C-based device communication and processing.
 *
 * The pList struct includes members for maintaining I2C bus configuration, addresses for
 * connected devices, handles for various modules, and parameters for system settings and
 * operational modes.
 *
 * @typedef tag_pList pList
 *
 * @var pList::adapter
 * pololu_i2c_adapter instance that handles I2C communication.

 * @var pList::ppsHandle
 * Handle for pulse-per-second (PPS) signal management.

 * @var pList::edge_cb_id
 * Identifier for edge callback management.

 * @var pList::magHandle
 * Handle for the magnetometer device.

 * @var pList::localTempHandle
 * Handle for the local temperature sensor.

 * @var pList::remoteTempHandle
 * Handle for the remote temperature sensor.

 * @var pList::magnetometerAddr
 * Address of the magnetometer device on the I2C bus.

 * @var pList::localTempAddr
 * Address of the local temperature sensor on the I2C bus.

 * @var pList::remoteTempAddr
 * Address of the remote temperature sensor on the I2C bus.

 * @var pList::doBistMask
 * Mask value for Built-In Self Test (BIST) configuration.

 * @var pList::cc_x
 * Coefficient for 'x' component.

 * @var pList::cc_y
 * Coefficient for 'y' component.

 * @var pList::cc_z
 * Coefficient for 'z' component.

 * @var pList::x_gain
 * Gain factor for the x-axis.

 * @var pList::y_gain
 * Gain factor for the y-axis.

 * @var pList::z_gain
 * Gain factor for the z-axis.

 * @var pList::XYZ
 * Array for storing XYZ components, size is 9.

 * @var pList::TMRCRate
 * Timer sampling rate.

 * @var pList::CMMSampleRate
 * Common Mode Sample Rate.

 * @var pList::samplingMode
 * Flag indicating the mode of sampling for sensors.

 * @var pList::NOSRegValue
 * Value for the NOS register.

 * @var pList::DRDYdelay
 * Delay for Data Ready (DRDY) signal.

 * @var pList::readBackCCRegs
 * Flag indicating whether to read back the CC registers.

 * @var pList::magRevId
 * Revision ID of the magnetometer device.

 * @var pList::i2cBusNumber
 * Identifier for the I2C bus number being utilized.

 * @var pList::tsMilliseconds
 * Timestamp value in milliseconds.

 * @var pList::Version
 * Pointer to a string representing the version information.

 * @var pList::usePipes
 * Flag indicating whether pipes are used.

 * @var pList::pipeInPath
 * Path to the input pipe file.

 * @var pList::pipeOutPath
 * Path to the output pipe file.
 */
typedef struct tag_pList
{
    int fd;
    char *portpath;
    pololu_i2c_adapter *adapter;

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
} pList;


//------------------------------------------
// Prototypes
//------------------------------------------
int  main(int argc, char** argv);
void onEdge(void);
char *formatOutput(pList *p, char *outBuf);
// int  initGPIO(volatile pList *p);
// void termGPIO(volatile pList *p);
int  verifyMagSensor(pList *p);
int  initMagSensor(pList *p);
int  initTempSensors(pList *p);

int  readLocalTemp(pList *p);
int  readRemoteTemp(pList *p);
//int  readMagCMM(volatile pList *p);
int  readMagPOLL(pList *p);

void showErrorMsg(int temp);

//void showPIGPIOErrMsg(int rv);

#endif //SWX3100MAIN_h

