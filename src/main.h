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
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include "MCP9808.h"
//#include "rm3100.h"

#ifndef TRUE
    #define TRUE  1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

//------------------------------------------
// Debugging output
//------------------------------------------
#define CONSOLE_OUTPUT  TRUE

//------------------------------------------
// Pi specific stuff
//------------------------------------------
#define USE_WAITFOREDGE     FALSE
#define FOR_GRAPE2          FALSE

//------------------------------------------
// ic stuff
//------------------------------------------
#define USE_PIGPIO          FALSE
#define USE_LGPIO           FALSE
#define USE_RGPIO           FALSE
#define USE_POLOLU          TRUE

#define USE_PIPES           FALSE

#if(USE_PIGPIO)
    #include "pigpio.h"
#endif
#if(USE_LGPIO)
    #include "lgpio.h"
#endif
#if(USE_POLOLU)
    #include "i2c-pololu.h"
#endif
#if(USE_RGPIO)
    #include "rgpio.h"
#endif

//------------------------------------------
// Macros and runtime options.
//------------------------------------------
#define OUTPUT_PRINT        stdout
#define OUTPUT_ERROR        stderr
#define PPS_TIMEOUTSECS     2.0

#define MAGDATA_VERSION     "0.2.0"
#define UTCBUFLEN           64
#define MAXPATHBUFLEN       1025
#define XYZ_BUFLEN          9

//------------------------------------------
// Control Parameter List struct
//------------------------------------------
typedef struct tag_pList
{
    int fd;
    char *portpath;
#if(USE_POLOLU)
    i2c_pololu_adapter *adapter;
#endif
    int scanI2CBUS;

    int ppsHandle;
    unsigned edge_cb_id;
    
    unsigned magHandle;
    unsigned localTempHandle;
    unsigned remoteTempHandle;

    int  magAddr;
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

#if(!USE_POLOLU)
    int  i2cBusNumber;
#endif
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
char *formatOutput(pList *p);
int  scanforBusDevices(pList *p);
int  verifyMagSensor(pList *p);
int  initMagSensor(pList *p);
int  initTempSensors(pList *p);
int  readLocalTemp(pList *p);
int  readRemoteTemp(pList *p);
//int  readMagCMM(volatile pList *p);
int  readMagPOLL(pList *p);

struct tm *getUTC();
void showErrorMsg(int temp);

#endif //SWX3100MAIN_h

