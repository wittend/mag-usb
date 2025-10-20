//=========================================================================
// main.c
//
// A command line interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// License:     GPL 3.0
// Note:        replaces i2c.c (using file system calls to read(), write(), etc.
//              with calls to pigpio. 
//              Also adding callbacks on GPIO27 for PPS rising edge.
//=========================================================================
#include <linux/limits.h>
#include "main.h"
#include "i2c.h"
#include "cmdmgr.h"
#include "rm3100.h"
#include "config.h"
#include "sensor_tests.h"

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

//------------------------------------------
// Macros
//------------------------------------------
#define PPS_GPIO_PIN    27
#define PPS_TIMEOUTSECS 2.0

//------------------------------------------
// Static and Global variables
//------------------------------------------
char Version[32];
int volatile killflag;
static char outBuf[256];
char portpath[PATH_MAX] = "/dev/ttyMAG0";          // default path for pololu i2c emulator.

// #ifdef USE_PIPES
//     char fifoCtrl[] = "/home/PSWS/Sstat/magctl.fifo";
//     char fifoData[] = "/home/PSWS/Sstat/magdata.fifo";
//     char fifoHome[] = "/run/user/";
//     int PIPEIN  = -1;
//     int PIPEOUT = -1;
// #endif //USE_PIPES
#if((USE_LGPIO || USE_RGPIO) && USE_WAITFOREDGE)
void cbTestFunc()
{
    printf("Got Callback!/n");
}

int wait_for_edge(int sbc, int handle, int gpio_pin, int edge, CBFunc_t f, int timeout)
{
    int rv = 0;
    rv = callback(sbc, handle, gpio_pin, edge, f, NULL);
    return rv;
}
int rc = 0;
// wait_for_edge((int)1, (int)1, PPS_GPIO_PIN, RISING_EDGE, cbTestFunc, PPS_TIMEOUTSECS);
#endif

//---------------------------------------------------------------
// Shared state between threads
//---------------------------------------------------------------
volatile sig_atomic_t shutdown_requested = 0;   // Signal-safe flag
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; // Protect shared data
int sensor_data = 0;                            // Simulated sensor data

//---------------------------------------------------------------
//  main()
//---------------------------------------------------------------
int main(int argc, char** argv)
{
    pList   ctl;
    pList   *p = &ctl;
    int     rv = 0;
    char    utcStr[UTCBUFLEN] = "";
    struct  tm *utcTime;

#if(USE_POLOLU)
    i2c_pololu_adapter pAdapter;
#endif

    //-----------------------------------------
    //  Setup magnetometer parameter defaults.
    //-----------------------------------------
    memset(p, 0, sizeof(pList));
    setProgramDefaults(p);
#if(USE_POLOLU)
    p->use_I2C_converter    = 1;
    p->adapter              = &pAdapter;
#else
    p->i2cBusNumber         = RASPI_I2C_BUS1;
#endif

    //-----------------------------------------
    //  Load configuration from TOML file
    //  (command line args will override these)
    //-----------------------------------------
    load_config("config.toml", p);

    if((rv = getCommandLine(argc, argv, p)) != 0)
    {
        return rv;
    }

// #if(USE_PIPES)
//    setupPipes(p);
// #endif // USE_PIPES
//    unsigned edge_cb_id = 0;
    
    i2c_init(p);
    i2c_open(p, portpath);

#if(USE_POLOLU)
    if(p->checkPololuAdaptor)
    {
        if(verifyPololuAdaptor(p) < 0)
        {
            exit(1002);
        }
        else
        {
    //        getAdaptorInfo(p);
        }
     }

    //-----------------------------------------------------
    // Get interface info and scan for i2c devices.
    //-----------------------------------------------------
    if(p->scanI2CBUS)
    {
        if(scanforBusDevices(p) <= 0)
        {
            //exit(1001);
            return -1;
        }
    }
#endif
    //-----------------------------------------------------
    // Verify the Temp sensor presence and Version.
    //-----------------------------------------------------
    if(p->checkTempSensor)
    {
        if(verifyTempSensor(p))
        {
     //       exit(1002);
        }
    }

    //-----------------------------------------
    // Verify the Mag sensor presence and Version.
    //-----------------------------------------
    if(p->checkMagSensor)
    {
        if(verifyMagSensor(p))
        {
            utcTime = getUTC();
            strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
            fprintf(OUTPUT_ERROR, "    {ts: \"%s\", lastStatus: \"Unable to Verify the magnetometer.\"}", utcStr);
            fflush(OUTPUT_ERROR);
            exit(2);
        }
    }

    //-----------------------------------------
    //  Initialize the Mag sensor registers.
    //-----------------------------------------
    initMagSensor(p);
    // if(initMagSensor(p))
    // {
    //     utcTime = getUTC();
    //     strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
    //     fprintf(OUTPUT_ERROR, "    {ts: \"%s\", lastStatus: \"Unable to initialize the magnetometer.\"}", utcStr);
    //     fflush(OUTPUT_ERROR);
    //     exit(2);
    // }

    //-----------------------------------------------------
    //  Main program loop.
    //-----------------------------------------------------
#if(USE_PTHREADS)
    pthread_t sensor_thread, print_thread, signal_thread;
    fprintf(OUTPUT_PRINT, "\n");
    // Create threads
    if (pthread_create(&sensor_thread, NULL, read_sensors, (void *) p) != 0)
    {
        perror("pthread_create sensor");
        exit(1);
    }
    if (pthread_create(&print_thread, NULL, print_data, (void *) p) != 0)
    {
        perror("pthread_create print");
        exit(1);
    }
    if (pthread_create(&signal_thread, NULL, signal_handler_thread, NULL) != 0)
    {
        perror("pthread_create signal");
        exit(1);
    }
    // Wait for signal handler thread to complete (it will only return on signal)
    pthread_join(signal_thread, NULL);
    // Signal handler has set shutdown_requested, so wait for other threads to finish
    pthread_join(sensor_thread, NULL);
    pthread_join(print_thread, NULL);
    // Clean up
    pthread_mutex_destroy(&data_mutex);
#else

    while(1)
    {
        // if(PPS_Flag)
        // {
        //     PPS_Flag = 0;
        //     //formatOutput(p, outBuf);
        //     formatOutput(p);
        //     fflush(outfp);
        // }
#if(USE_WAITFOREDGE)
        if(!(rv = wait_for_edge(p->po, (unsigned) PPS_GPIO_PIN, RISING_EDGE, PPS_TIMEOUTSECS)))
        {
            utcTime = getUTC();
            strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);                // RFC 2822: "%a, %d %b %Y %T %z"      RFC 822: "%a, %d %b %y %T %z"
    #if(CONSOLE_OUTPUT)
            fprintf(OUTPUT_PRINT, "   [CHILD]: {ts: \"%s\", lastStatus: \"Missed PPS Timeout!\"}", utcStr);
            fflush(OUTPUT_PRINT);
    #else
            char outstr[MAXPATHBUFLEN] = "";
            sprintf(outstr, "   [CHILD]: {ts: \"%s\", lastStatus: \"Missed PPS Timeout!\"}", utcStr);
            write(PIPEOUT, outstr);
    #endif
            // Set exit return value.
            rv = 2;
            break;
        }
#endif
#if __DEBUG
        else
        {
            fputs(".", OUTPUT_PRINT);
            fflush(OUTPUT_PRINT);
        }
#endif
    }
    //-----------------------------------------------------
    //  Cleanup Callback, PIGPIO, and exit.
    //-----------------------------------------------------
    #if(USE_RGPIO || USE_LGPIO)
        rv = callback_cancel(p->edge_cb_id);
    #elif (USE_PIGPIO)
        rv = event_callback_cancel(p->edge_cb_id);
    #endif
#endif // USE_PTHREADS
    printf("Program terminated.\n");
    return 0;
}

//---------------------------------------------------------------
// Function to simulate reading sensor data
//---------------------------------------------------------------
void* read_sensors(void* arg)
{
    while (!shutdown_requested)
    {
        // Sleep for 1000 ms (1 second)
        usleep(1000000);

        // Check for shutdown request
        if (shutdown_requested)
        {
            break;
        }
    }
    return NULL;
}

//---------------------------------------------------------------
// Function to print sensor data every 1000 ms
//---------------------------------------------------------------
void* print_data(void* arg)
{
    while (!shutdown_requested)
    {
        pList * p = (pList *) arg;
        // Wait for 1000 ms or until shutdown is requested
        // struct timespec ts;
        // clock_gettime(CLOCK_REALTIME, &ts);
        // ts.tv_sec += 1; // Add 1 second
        //
        // // Use condition variable with timeout to wait
        // pthread_mutex_lock(&data_mutex);
        // int local_data = sensor_data;
        // pthread_mutex_unlock(&data_mutex);
        //
        // // Print data to stdout
        // printf("Sensor Data: %d\n", local_data);
        formatOutput(p);

        // Sleep for 1000 ms or until interrupted by signal
        struct timespec sleep_time = {1, 0}; // 1 second
        nanosleep(&sleep_time, NULL);
    }
    return NULL;
}

//---------------------------------------------------------------
// Signal handler thread function
//---------------------------------------------------------------
void* signal_handler_thread(void* arg)
{
    sigset_t sigset;
    int signum;

    // Block SIGHUP, SIGABRT, and SIGINT in this thread
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGABRT);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // Wait for any of the blocked signals
    while (1)
    {
        if (sigwait(&sigset, &signum) == 0)
        {
            switch (signum)
            {
                case SIGHUP:
                    printf("Received SIGHUP. Shutting down gracefully.\n");
                    break;
                case SIGABRT:
                    printf("Received SIGABRT. Shutting down gracefully.\n");
                    break;
                case SIGINT:
                    printf("Received SIGINT. Shutting down gracefully.\n");
                    break;
                default:
                    printf("Received unexpected signal %d.\n", signum);
                    break;
            }
            // Set shutdown flag
            shutdown_requested = 1;
            break;
        }
    }
    return NULL;
}

//---------------------------------------------------------------
// void formatOutput(volatile pList *p, char *outBuf)
//---------------------------------------------------------------
char *formatOutput(pList *p)
{
#define FMTBUFLEN  200
    char fmtBuf[FMTBUFLEN + 1] ="";
    int fmtBuf_len      = sizeof fmtBuf;
    struct tm *utcTime  = getUTC();
    char utcStr[128]    ="";
    double xyz[3];
    double rcRemoteTemp;

    strncpy(outBuf, "", 1);

    readMagPOLL(p);

    xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain) * 1000; // make microTeslas -> nanoTeslas
    xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain) * 1000; // make microTeslas -> nanoTeslas
    xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain) * 1000; // make microTeslas -> nanoTeslas

    // xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain);
    // xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain);
    // xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain);

#if(FOR_GRAPE2)
    strftime(utcStr, UTCBUFLEN, "%Y%m%e%y%M%S", utcTime);              // YYYYMMDDHHMMSS  (Gaak!)
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":%s", utcStr);
#else
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);                // RFC 2822: "%a, %d %b %Y %T %z"
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":\"%s\"", utcStr);
#endif

    rcRemoteTemp = readTemp(p);

    utcTime = getUTC();
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);                // RFC 2822: "%a, %d %b %Y %T %z"
    snprintf(fmtBuf, fmtBuf_len, "{ \"ts\":\"%s\"", utcStr);
    strncat(outBuf, fmtBuf, FMTBUFLEN);

    if(rcRemoteTemp < -100.0)
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"rt\":0.0");
        strncat(outBuf, fmtBuf, FMTBUFLEN);
    }
    else
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"rt\":%.2f",  rcRemoteTemp);
        strncat(outBuf, fmtBuf, FMTBUFLEN);
    }

    snprintf(fmtBuf, fmtBuf_len, ", \"x\":%.3f", xyz[0]);
    strncat(outBuf, fmtBuf, FMTBUFLEN);
    snprintf(fmtBuf, fmtBuf_len, ", \"y\":%.3f", xyz[1]);
    strncat(outBuf, fmtBuf, FMTBUFLEN);
    snprintf(fmtBuf, fmtBuf_len, ", \"z\":%.3f", xyz[2]);
    strncat(outBuf, fmtBuf, FMTBUFLEN);

    snprintf(fmtBuf, fmtBuf_len, " }\n");
    strncat(outBuf, fmtBuf, FMTBUFLEN);

#if(CONSOLE_OUTPUT)
    fprintf(OUTPUT_PRINT, " %s", outBuf);
    fflush(OUTPUT_PRINT);
// #elif(USE_PIPES)
//     write(PIPEOUT, outBuf);
#else    
    fprintf(OUTPUT_PRINT, "  %s", outBuf);
    fflush(OUTPUT_PRINT);
#endif
    return outBuf;
}

//---------------------------------------------------------------
// readTemp(pList *p)
//---------------------------------------------------------------
double readTemp(pList *p)
{
    uint8_t temp_buf[2] = {0xFF, 0xFF};

    int rv = i2c_pololu_read_from(p->adapter, p->remoteTempAddr, MCP9808_REG_AMBIENT_TEMP, temp_buf, 2);
    if (rv < 2)
    {
        char usingcall[256] = "i2c_pololu_read_from";
        char ebuf[300] = "";
        snprintf(ebuf,sizeof(ebuf),"Read with: %s() ", usingcall);
        perror(ebuf);
    }
    double celsius = mcp9808_decode_celsius(temp_buf[0], temp_buf[1]);
    return celsius;
}

//------------------------------------------
//  mcp9808_decode_celsius()
//------------------------------------------
static double mcp9808_decode_celsius(uint8_t msb, uint8_t lsb)
{
    uint16_t raw = ((uint16_t)msb << 8) | lsb;
    // First, clear alert flag bits (15, 14, 13)
    raw &= 0x1FFF;  // Keep only bits 12-0

    double c;
    if (raw & 0x1000)
    {
        // Bit 12 (sign bit) is set - negative temperature
        // For two's complement of a 13-bit signed value:
        // Treat bits 12-0 as signed, or manually compute
        c = -(double)(raw & 0x0FFF) / 16.0 - 256.0;
    }
    else
    {
        // Positive temperature
        c = (double)(raw & 0x0FFF) / 16.0;
    }
    return c;
}

//------------------------------------------
// readMagPOLL()
//------------------------------------------
int readMagPOLL(pList *p)
{
    int     rv = 0;
    //int     bytes_read = XYZ_BUFLEN;
    int     bytes_read = 0;
    char    xyzBuf[XYZ_BUFLEN] = "";

    // Read back XYZ_BUFLEN + 1 bytes (skip the first byte).
    rv = i2c_pololu_read_from(p->adapter, p->magAddr, RM3100_MAG_POLL, xyzBuf, (XYZ_BUFLEN));       //(XYZ_BUFLEN + 1)
    if(rv < 0)
    {
        fprintf(stdout, "  Read failed: %s\n", i2c_pololu_error_string(-rv));
        goto done;
    }
    if(rv < (XYZ_BUFLEN))
    {
        // Wait for DReady Flag.
        rv = i2c_pololu_read_from(p->adapter, p->magAddr, RM3100_MAG_POLL, xyzBuf, (XYZ_BUFLEN));
        rv = (rv & RM3100I2C_READMASK);
        while((rv != RM3100I2C_READMASK))
        {
            rv = i2c_pololu_read_from(p->adapter, p->magAddr, RM3100_MAG_POLL, xyzBuf, (XYZ_BUFLEN));
            rv = (rv & RM3100I2C_READMASK);
        }
    }
    else if(rv == XYZ_BUFLEN)
    {
        p->XYZ[0] = ((signed char)xyzBuf[0]) * 256 * 256;
        p->XYZ[0] |= xyzBuf[1] * 256;
        p->XYZ[0] |= xyzBuf[2];

        p->XYZ[1] = ((signed char)xyzBuf[3]) * 256 * 256;
        p->XYZ[1] |= xyzBuf[4] * 256;
        p->XYZ[1] |= xyzBuf[5];

        p->XYZ[2] = ((signed char)xyzBuf[6]) * 256 * 256;
        p->XYZ[2] |= xyzBuf[7] * 256;
        p->XYZ[2] |= xyzBuf[8];
    }
    else
    {
        showErrorMsg(rv);
    }
    return bytes_read;
done:
    return 0;
}

//---------------------------------------------------------------
// int initMagSensor(volatile pList *p)
//---------------------------------------------------------------
int initMagSensor(pList *p)
{
    int rv = 0;

    // Setup the Mag sensor register initial state here.
    if(p->samplingMode == POLL)                                         // (p->samplingMode == POLL [default])
    {
        //if((rv = i2c_pololu_write_to(p->adapter, addr, reg, "", 1))) !=
        //if((rv = i2c_writebyte_mag(p, RM3100_MAG_POLL, "", 1)) != 1)
        PMMODE_ALL
        rv = i2c_pololu_read_from(p->adapter, p->magAddr, RM3100_MAG_POLL, xyzBuf, (XYZ_BUFLEN));       //(XYZ_BUFLEN + 1)
        if(rv < 0)
        {
            fprintf(stdout, "  Read failed: %s\n", i2c_pololu_error_string(-rv));
            goto done;
        }
        {
            showErrorMsg(rv);
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "    initMagSensor(POLL) FAILS.\n");
            fflush(OUTPUT_PRINT);
#endif
        }
    }
    else
    {
        if((rv = i2c_writebyte_mag(p, RM3100I2C_CMM, "", 1)) != 1)
        {
            showErrorMsg(rv);
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "    [Child]: initMagSensor(CMM) FAILS.\n");
            fflush(OUTPUT_PRINT);
#endif
        }
    }
    return FALSE;
}

// //---------------------------------------------------------------
// // int initTempSensors(volatile pList *p)
// //---------------------------------------------------------------
// int initTempSensors(pList *p)
// {
//     int rv = 0;
//     // Temp sensor doesn't need any iniutialization currently.
//     return rv;
// }

//------------------------------------------
// getUTC()
//------------------------------------------
struct tm *getUTC()
{
    time_t now = time(&now);
    if(now == -1)
    {
        puts("The time() function failed");
    }
    struct tm *ptm = gmtime(&now);
    if(ptm == NULL)
    {
        puts("The gmtime() function failed");
    }
    return ptm;
}

//------------------------------------------
// currentTimeMillis()
//------------------------------------------
long currentTimeMillis()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

//------------------------------------------
// int setupPipes(pList *p)
//------------------------------------------
//int setupPipes(pList *p)
//{
//     //-----------------------------------------
//     //  Setup the I/O pipes
//     //-----------------------------------------
//     int  fdPipeIn;
//     int  fdPipeOut;
//
//     if(p->usePipes == TRUE)
//     {
//         // Notice that fdPipeOut and fdPipeIn are intentionally reversed.
//         if(!(fdPipeOut = open(p->pipeInPath, O_WRONLY | O_CREAT)))
//         {
//             perror("    [CHILD] Open PIPE Out failed: ");
//             fprintf(OUTPUT_PRINT, "%s", p->pipeInPath);
//             exit(1);
//         }
//         else
//         {
//             fprintf(OUTPUT_PRINT, "    [CHILD] Open PIPE Out OK.\n");
//             fflush(OUTPUT_PRINT);
//             PIPEOUT = fdPipeOut;
//         }
//
//         if(!(fdPipeIn = open(p->pipeOutPath, O_RDONLY | O_CREAT)))
//         {
//             perror("    [CHILD] Open PIPE In failed: ");
//             fprintf(OUTPUT_PRINT, "%s", p->pipeInPath);
//             exit(1);
//         }
//         else
//         {
//             fprintf(OUTPUT_PRINT, "    [CHILD] Open PIPE In OK.\n");
//             fflush(OUTPUT_PRINT);
//             PIPEIN = fdPipeIn;
//         }
//     }
//}

//------------------------------------------
// int setProgramDefaults(pList *p)
//------------------------------------------
void setProgramDefaults(pList *p)
{
// #if(USE_POLOLU)
//     i2c_pololu_adapter pAdapter;
// #endif
    p->portpath             = portpath;
    p->scanI2CBUS           = FALSE;
    p->checkPololuAdaptor   = FALSE;
    p->checkMagSensor       = FALSE;
    p->checkTempSensor      = FALSE;
    p->ppsHandle            = 0;
    p->magHandle            = 0;
    p->remoteTempHandle     = 0;
    p->doBistMask           = 0;
    p->cc_x                 = (int) CC_400;
    p->cc_y                 = (int) CC_400;
    p->cc_z                 = (int) CC_400;
    p->x_gain               = GAIN_150;
    p->y_gain               = GAIN_150;
    p->z_gain               = GAIN_150;
    p->tsMilliseconds       = 0;
    p->TMRCRate             = 0x96;
    p->Version              = Version;
    p->samplingMode         = POLL;
    p->readBackCCRegs       = FALSE;
    p->CMMSampleRate        = 400;
    p->NOSRegValue          = 60;
    p->DRDYdelay            = 10;
    p->magRevId             = 0x0;
    p->remoteTempAddr       = 0x1F;
    p->magAddr              = RM3100_I2C_ADDRESS;
    //    p->usePipes             = USE_PIPES;
    //    p->pipeInPath           = fifoCtrl;
    //    p->pipeOutPath          = fifoData;
    p->readBackCCRegs       = FALSE;
}

#if(USE_RGPIO | USE_LGPIO | USE_PIGPIO)
//---------------------------------------------------------------
// void onEdge(void)
//---------------------------------------------------------------
void onEdge(void)
{
#if(__DEBUG)
    fputs("|", OUTPUT_PRINT);
    fflush(OUTPUT_PRINT);
#endif
    PPS_Flag = TRUE;
}
#endif
