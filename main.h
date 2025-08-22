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
#include "src/i2c-pololu.h"

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
typedef struct tag_ctlList
{
    int po;
//    pololu_i2c_adapter po->adapter;
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

