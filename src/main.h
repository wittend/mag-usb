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
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <limits.h>
#include "MCP9808.h"

#ifndef TRUE
    #define TRUE  1
#endif
#ifndef FALSE
    #define FALSE 0
#endif

//------------------------------------------
// Debugging output
//------------------------------------------
#define __DEBUG             TRUE
#define CONSOLE_OUTPUT      TRUE

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

//#define USE_PIPES            FALSE
//#undef USE_PIPES             FALSE
#define USE_PTHREADS         TRUE

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

#define MAGDATA_VERSION     "0.0.3"
#define UTCBUFLEN           64
#define MAXPATHBUFLEN       PATH_MAX
#define XYZ_BUFLEN          9

#define POLL                0
#define CMM                 1

//------------------------------------------
// Control Parameter List struct
//------------------------------------------
typedef struct tag_pList
{
    int fd;
    char *portpath;
//#if(USE_POLOLU)
    int use_I2C_converter;
    i2c_pololu_adapter *adapter;
//#else
    int i2cBusNumber;
//#endif
    int checkPololuAdaptor;
    int scanI2CBUS;
    int checkTempSensor;
    int checkMagSensor;

    int ppsHandle;
    unsigned edge_cb_id;
    
    unsigned magHandle;
    unsigned remoteTempHandle;

    int  magAddr;
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

    int  tsMilliseconds;
    int  usePipes;
    int  write_logs;
    int  create_log_path_if_empty;

    char *maintainer;
    char *maintainer_email;
    char *Version;
    char *pipeInPath;
    char *pipeOutPath;
    char *log_output_path;
    char *latitude;
    char *longitude;
    char *elevation;
    char *grid_square;

    int  showSettingsOnly; // if set by -O, print settings and exit
 } pList;

//------------------------------------------
// Prototypes
//------------------------------------------
int  main(int argc, char** argv);
char *formatOutput(pList *p);
void* read_sensors(void* arg);
void* print_data(void* arg);
void* signal_handler_thread(void* arg);
double readTemp(pList *p);
void showErrorMsg(int temp);
void setProgramDefaults(pList *p);

long currentTimeMillis();
struct tm *getUTC();
// int setupPipes(pList *p);

#endif //SWX3100MAIN_h

