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

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

//------------------------------------------
// Debugging output
//------------------------------------------
// see main.h
//
//#define __DEBUG         FALSE

//------------------------------------------
// Macros
//------------------------------------------
#define PPS_GPIO_PIN    27
#define PPS_TIMEOUTSECS 2.0

//------------------------------------------
// Static and Global variables
//------------------------------------------
// char Version[32]        = MAGDATA_VERSION;
// int volatile PPS_Flag   = 0;
// int volatile killflag   = 0;
// static char outBuf[256] = "";
// char portpath[PATH_MAX] = "/dev/ttyACM0";          // default path for pololu i2c emulator.
char Version[32];
int volatile PPS_Flag = FALSE;
int volatile killflag;
static char outBuf[256];
char portpath[PATH_MAX] = "/dev/ttyACM0";          // default path for pololu i2c emulator.

#ifdef USE_PIPES
    char fifoCtrl[] = "/home/pi/PSWS/Sstat/magctl.fifo";
    char fifoData[] = "/home/pi/PSWS/Sstat/magdata.fifo";
    char fifoHome[] = "/run/user/";
    int PIPEIN  = -1;
    int PIPEOUT = -1;
#endif //USE_PIPES

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
volatile sig_atomic_t shutdown_requested = 0; // Signal-safe flag
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; // Protect shared data
int sensor_data = 0; // Simulated sensor data

//---------------------------------------------------------------
//  main()
//---------------------------------------------------------------
int main(int argc, char** argv)
{
    pList   ctl;
    pList   *p = &ctl;
#if(USE_POLOLU)
    i2c_pololu_adapter pAdapter;
#endif

    int     rv = 0;
    FILE    *outfp = (FILE *)stdout;
    char    utcStr[UTCBUFLEN] = "";
    struct  tm *utcTime = getUTC();
    //char    portpath[MAXPATHBUFLEN];

#if(__DEBUG)
    #if(USE_PIPES)
        printf("fifoCtrl: %s\n", fifoCtrl);
        printf("fifoData: %s\n", fifoData);
    #endif
    fprintf(OUTPUT_PRINT, "\n[CHILD] In %s child process.\n", argv[0]);
    fflush(stdout);
#endif

    //-----------------------------------------
    //  Setup magnetometer parameter defaults.
    //-----------------------------------------
    memset(p, 0, sizeof(pList));

    p->portpath         = portpath;
    p->scanI2CBUS       = FALSE;
#if(USE_POLOLU)
    p->adapter          = &pAdapter;
#else
    p->i2cBusNumber     = RASPI_I2C_BUS1;
#endif
    p->ppsHandle        = 0;
    p->magHandle        = 0;
    p->localTempHandle  = 0;
    p->remoteTempHandle = 0;
    p->doBistMask       = 0;
    p->cc_x             = (int) CC_400;
    p->cc_y             = (int) CC_400;
    p->cc_z             = (int) CC_400;
    p->x_gain           = GAIN_150;
    p->y_gain           = GAIN_150;
    p->z_gain           = GAIN_150;
    p->tsMilliseconds   = 0;
    p->TMRCRate         = 0x96;
    p->Version          = Version;
    p->samplingMode     = POLL;
    p->readBackCCRegs   = FALSE;
    p->CMMSampleRate    = 400;
    p->NOSRegValue      = 60;
    p->DRDYdelay        = 10;
    p->magRevId         = 0x0;
    p->remoteTempAddr   = 0x1F;
    p->magAddr          = RM3100_I2C_ADDRESS;
    p->usePipes         = USE_PIPES;
    p->pipeInPath       = fifoCtrl;
    p->pipeOutPath      = fifoData;
    p->readBackCCRegs   = FALSE;

    if((rv = getCommandLine(argc, argv, p)) != 0)
    {
        return rv;
    }

#if(USE_PIPES)
    //-----------------------------------------
    //  Setup the I/O pipes
    //-----------------------------------------
    int  fdPipeIn;
    int  fdPipeOut;

    if(p->usePipes == TRUE)
    {
        // Notice that fdPipeOut and fdPipeIn are intentionally reversed.
        if(!(fdPipeOut = open(p->pipeInPath, O_WRONLY | O_CREAT)))
        {
            perror("    [CHILD] Open PIPE Out failed: ");
            fprintf(OUTPUT_PRINT, "%s", p->pipeInPath);
            exit(1);
        }
        else
        {
            fprintf(OUTPUT_PRINT, "    [CHILD] Open PIPE Out OK.\n");
            fflush(OUTPUT_PRINT);
            PIPEOUT = fdPipeOut;
        }

        if(!(fdPipeIn = open(p->pipeOutPath, O_RDONLY | O_CREAT)))
        {    
            perror("    [CHILD] Open PIPE In failed: ");
            fprintf(OUTPUT_PRINT, "%s", p->pipeInPath);
            exit(1);
        }
        else
        {
            fprintf(OUTPUT_PRINT, "    [CHILD] Open PIPE In OK.\n");
            fflush(OUTPUT_PRINT);
            PIPEIN = fdPipeIn;
        }
    }
#endif // USE_PIPES

//    unsigned edge_cb_id = 0;
    
#if(___DEBUG)
    fprintf(OUTPUT_PRINT, "    [CHILD] Before setting up GPIO.\n");
    fflush(OUTPUT_PRINT);
#endif

    // printf("Connecting to %s...\n", port_name);
    // if(pololu_i2c_connect(&adapter, port_name) != 0)
    // {
    //     fprintf(stderr, "Failed to connect to the adapter.\n");
    //     return 1;
    // }
    // printf("Connected.\n");
    i2c_init(p);
    i2c_open(p, portpath);

#if(USE_POLOLU)
    //-----------------------------------------------------
    // Get interface info and scan for i2c devices.
    //-----------------------------------------------------
    if(p->scanI2CBUS)
    {
        if(scanforBusDevices(p) <= 0)
        {
            exit(1001);
        }
    }
#endif

    //-----------------------------------------
    // Verify the Mag sensor presence and Version.
    //-----------------------------------------
    if(verifyMagSensor(p))
    {
        utcTime = getUTC();
        strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
        fprintf(OUTPUT_ERROR, "    [CHILD] {ts: \"%s\", lastStatus: \"Unable to Verify the magnetometer.\"}", utcStr); 
        fflush(OUTPUT_ERROR);
        exit(2);
    }
    else
    {
        //-----------------------------------------
        //  Initialize the Mag sensor registers.
        //-----------------------------------------
        if(initMagSensor(p))
        {
            utcTime = getUTC();
            strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
            fprintf(OUTPUT_ERROR, "    [CHILD] {ts: \"%s\", lastStatus: \"Unable to initialize the magnetometer.\"}", utcStr); 
            fflush(OUTPUT_ERROR);
            exit(2);
        }
    }
    // //-----------------------------------------------------
    // //  Initialize the Temp sensor registers.
    // //-----------------------------------------------------
    // if(initTempSensors(p))
    // {
    //     utcTime = getUTC();
    //     strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
    //     fprintf(OUTPUT_ERROR, "    [CHILD] {ts: \"%s\", lastStatus: \"Unable to initialize the temperature sensor.\"}", utcStr);
    //     fflush(OUTPUT_ERROR);
    //     exit(1);
    // }
// return 0;
#if ___DEBUG
    fprintf(OUTPUT_PRINT, "    [CHILD] Before setting up callback to: onEdge() for PPS...\n");
    fflush(OUTPUT_PRINT);
#endif
    
    //-----------------------------------------------------
    //  Main program loop.
    //-----------------------------------------------------

#if(USE_PTHREADS)

    fprintf(OUTPUT_PRINT, "\n");
    pthread_t sensor_thread, print_thread, signal_thread;


    // Create threads
    if (pthread_create(&sensor_thread, NULL, read_sensor, (void *) p) != 0)
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
void* read_sensor(void* arg)
{
    while (!shutdown_requested)
    {
        pList * p = (pList *) arg;
        // Simulate sensor reading (replace with actual sensor code)
        sensor_data = rand() % 100; // Random data between 0 and 99

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
//char *formatOutput(pList *p, char *outBuf)
char *formatOutput(pList *p)
{
#define FMTBUFLEN  200
    char fmtBuf[FMTBUFLEN+1] ="";
    int fmtBuf_len      = sizeof fmtBuf;
    struct tm *utcTime  = getUTC();
    char utcStr[128]    ="";
    double xyz[3];
//    int localTemp       = 0;
    int remoteTemp      = 0;
    float rcLocalTemp   = 0.0;
    double rcRemoteTemp = 0.0;

    strncpy(outBuf, "", 1);

#if(__DEBUG)
    fprintf(OUTPUT_PRINT, "\n    [Child]: formatOutput()...\n");
    fflush(OUTPUT_PRINT);
#endif


/*
    // readMagPOLL(p);

//    xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain) * 1000; // make microTeslas -> nanoTeslas
//    xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain) * 1000; // make microTeslas -> nanoTeslas
//    xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain) * 1000; // make microTeslas -> nanoTeslas

    xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain);
    xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain);
    xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain);



    // readMagPOLL(p);
#if(FOR_GRAPE2)
    strftime(utcStr, UTCBUFLEN, "%Y%m%e%y%M%S", utcTime);              // YYYYMMDDHHMMSS  (Gaak!)
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":%s", utcStr);
#else
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);                // RFC 2822: "%a, %d %b %Y %T %z"
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":\"%s\"", utcStr);
#endif
*/
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

    strncat(outBuf, fmtBuf, FMTBUFLEN);
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
#elif(USE_PIPES)
    write(PIPEOUT, outBuf);
#else    
    fprintf(OUTPUT_PRINT, "  %s", outBuf);
    fflush(OUTPUT_PRINT);
#endif
    return outBuf;
}

// //------------------------------------------
// // readLocalTemp(volatile pList *p)
// //------------------------------------------
// int readLocalTemp(pList *p)
// {
//     int temp = -9999;
//     char data[2] = {0};
//
// #if(__DEBUG)
//     fprintf(OUTPUT_PRINT, "[Child]: readLocalTemp()...\n");
//     fflush(OUTPUT_PRINT);
// #endif
//
//     //if((temp = pololu_i2c_read_from((pololu_i2c_adapter *)p->po, p->localTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
//     if((temp = i2c_read_temp(p, MCP9808_REG_AMBIENT_TEMP) <= 0))
//     {
//         fprintf(OUTPUT_ERROR, "Error : I/O error reading temp sensor at address: [0x%2X].\n", MCP9808_REG_AMBIENT_TEMP);
//         showErrorMsg(temp);
//     }
//     else
//     {
//         // Convert the data to 13-bits
//         temp = ((data[0] & 0x1F) * 256 + data[1]);
//         if(temp > 4095)
//         {
//             temp -= 8192;
//         }
//     }
//     return temp;
// }

// //------------------------------------------
// // readRemoteTemp(volatile pList *p)
// //------------------------------------------
// int readRemoteTemp(pList *p)
// {
//     int temp = -9999;
//     char data[2] = {0};
//
// #if(__DEBUG)
//     fprintf(OUTPUT_PRINT, "[Child]: readRemoteTemp()...\n");
//     fflush(OUTPUT_PRINT);
// #endif
//
// //    if((temp = pololu_i2c_read_from((pololu_i2c_adapter *)adapter, p->localTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
// //    if((temp = i2c_readbuf(p->remoteTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
//     if((temp = i2c_readbuf_temp(p, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
//     {
//         fprintf(OUTPUT_ERROR, "Error : I/O error reading temp sensor at address: [0x%2X].\n", MCP9808_REG_AMBIENT_TEMP);
//         showErrorMsg(temp);
//     }
//     else
//     {
//         // Convert the data to 13-bits
//         temp = ((data[0] & 0x1F) * 256 + data[1]);
//         if(temp > 4095)
//         {
//             temp -= 8192;
//         }
//     }
//     return temp;
// }

double readTemp(pList *p)
{
#if(___DEBUG)
    fprintf(stdout,"\nReading MCP9808 at:  [0x%X] from MCP9808_REG_AMBIENT_TEMP: [0x%X].\n", p->remoteTempAddr, MCP9808_REG_AMBIENT_TEMP);
#endif
    uint8_t temp_buf[2] = {0xFF, 0xFF};

    int rv = i2c_pololu_read_from(p->adapter, p->remoteTempAddr, MCP9808_REG_AMBIENT_TEMP, temp_buf, 2);
    char usingcall[256] = "i2c_pololu_read_from";
    // rv = i2c_pololu_write_and_read_from(&adapter, MCP9808_LCL_I2CADDR_DEFAULT, MCP9808_REG_AMBIENT_TEMP, temp_buf, 2);
    // char usingcall[256] = "i2c_pololu_write_and_read_from";
#if(___DEBUG)
    fprintf(stdout,"Using %s():\n", usingcall);
    fprintf(stdout,"Read from register at: [0x%02X] returns: %i\n", MCP9808_REG_AMBIENT_TEMP, rv);
    fprintf(stdout,"Data temp_buf[0][1]: [0x%02X][0x%02X] from sensor\n", temp_buf[0], temp_buf[1]);
    {
        fprintf(stdout,"[0x%02X] [0x%02X]\n", temp_buf[0], temp_buf[1]);  // byte swapped ?
    }
    #endif
    if(rv < 0)
    {
        char ebuf[300] = "";
        snprintf(ebuf,sizeof(ebuf),"Read with: %s() ", usingcall);
        perror(ebuf);
    }
    double celsius = mcp9808_decode_celsius(temp_buf[0], temp_buf[1]);
#if(___DEBUG)
    fprintf(stdout,"MCP9808 ambient temperature: %.4f C\n", celsius);
#endif
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
    int     bytes_read = 9;
    short   pmMode = (PMMODE_ALL);

    char    xyzBuf[XYZ_BUFLEN] = "";

#if(__DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: readMagPOLL()...\n");
    fflush(OUTPUT_PRINT);
#endif

    // Write command to  use Polled measurement Mode.
//    rv = i2c_read_temp(p, MCP9808_REG_AMBIENT_TEMP);
    rv = i2c_readbuf_mag(p, RM3100_MAG_POLL, xyzBuf, 3 );
    if(rv != 0)
    {
        showErrorMsg(rv);
#if(__DEBUG)
        fprintf(OUTPUT_PRINT, "    [Child]: Write POLL mode < 0.\n");
        fflush(OUTPUT_PRINT);
#endif
        usleep(p->DRDYdelay);
    }
#if(__DEBUG)
    else
    {
        fprintf(OUTPUT_PRINT, "    [Child]: Write RM3100_MAG_POLL mode == %u -- OK.\n", rv);
        fflush(OUTPUT_PRINT);
    }
#endif

    // If a delay is specified after DRDY goes high, sleep it off.
    if(p->DRDYdelay)
    {
        usleep(p->DRDYdelay);
    }

#if(__DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: Before Write to RM3100I2C_XYZ.\n");
    fflush(OUTPUT_PRINT);
#endif

    // Tell the sensor that you want to read XYZ data. RM3100I2C_POLLXYZ
    // rv = i2c_write_byte_data(p->pi, p->magHandle, RM3100I2C_XYZ, TRUE);
    rv = i2c_write_temp(p, RM3100I2C_MX, 1);
//    rv = pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t 1 );
    rv = i2c_readbuf_mag(p, RM3100I2C_XYZ, xyzBuf, XYZ_BUFLEN );
    if(rv < 0)
    {
        showErrorMsg(p->magHandle);
    }
    else if(rv == 0)
    {
        // Wait for DReady Flag.
        //rv = i2c_readbyte(p, p->magHandle);
        rv = i2c_read_mag(p, p->magHandle);
        rv = (rv & RM3100I2C_READMASK);
        while((rv != RM3100I2C_READMASK))
        {
    //        rv = i2c_readbyte(p, p->magHandle);
            rv = i2c_read_mag(p, p->magHandle);
            rv = (rv & RM3100I2C_READMASK);
        }

#if(__DEBUG)
        fprintf(OUTPUT_PRINT, "    [Child]: Before Read RM3100I2C_XYZ. rv: %u\n", rv);
        fflush(OUTPUT_PRINT);
#endif

        // Read the data registers.
        //rv = i2c_read_device(p->pi, p->magHandle, xyzBuf, XYZ_BUFLEN);
        //rv = i2c_read_i2c_block_data(p->pi, p->magHandle, RM3100I2C_XYZ, (char *)xyzBuf, XYZ_BUFLEN);
        //rv = pololu_i2c_write_to( pololu_i2c_adapter *adapter, RM3100I2C_XYZ, (char *)xyzBuf, XYZ_BUFLEN);
        rv = i2c_readbuf_mag(p, RM3100I2C_XYZ, xyzBuf, XYZ_BUFLEN);
        if(rv == XYZ_BUFLEN)
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
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "\n    [Child]: readMagPOLL() -  Bytesread: %u.\n", bytes_read);
//            fprintf(OUTPUT_PRINT, "p->XYZ[0] : %.3X, p->XYZ[1]: %.3X, p->XYZ[2]: %.3X.\n", p->XYZ[0], p->XYZ[1], p->XYZ[2]);
            fflush(OUTPUT_PRINT);
#endif
        }
        else
        {
            showErrorMsg(rv);
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "\n    [Child]: readMagPOLL() -  Bytesread: %u.\n", bytes_read);
 //           fprintf(OUTPUT_PRINT, "p->XYZ[0] : %.3X, p->XYZ[1]: %.3X, p->XYZ[2]: %.3X.\n", p->XYZ[0], p->XYZ[1], p->XYZ[2]);
            fflush(OUTPUT_PRINT);
#endif
        }
    }
    return bytes_read;
}

//---------------------------------------------------------------
// scanforBusDevices(pList *p)
//---------------------------------------------------------------
int scanforBusDevices(pList *p)
{
    fprintf(stdout,"\nChecking Pololu I2C Adapter info:\n");

    //-----------------------------------------------------
    // Get the Pololu i2c device info.
    //-----------------------------------------------------
    i2c_pololu_device_info info;
    if(i2c_pololu_get_device_info(p->adapter, &info) == 0)
    {
        fprintf(stdout,"  Device Info:\n");
        fprintf(stdout,"     Vendor ID:        0x%04X\n", info.vendor_id);
        fprintf(stdout,"     Product ID:       0x%04X\n", info.product_id);
        fprintf(stdout,"     Firmware Version: %s\n", info.firmware_version);
        fprintf(stdout,"     Serial Number:    %s\n", info.serial_number);
    }
    else
    {
        fprintf(stdout, "    Failed to get device info.\n");
        return -1;
    }
    // Scan for I2C devices

    fprintf(stdout,"\n  Scanning for I2C devices...\n");
    uint8_t found_addresses[128];
    i2c_pololu_scan(p->adapter, found_addresses, 128);
    int device_count = i2c_pololu_scan(p->adapter, found_addresses, 128);
    if (device_count > 0)
    {
        printf("    Found %d device(s):\n", device_count);
        for (int i = 0; i < device_count; ++i)
        {
            fprintf(stdout,"      Address: 0x%02X\n", found_addresses[i]);
        }
        return device_count;
    }
    else if (device_count == 0)
    {
        fprintf(stderr,"      No I2C devices found.\n");
        return -1;
    }
    else
    {
        fprintf(stderr, "    An error occurred during the I2C scan: %s\n", i2c_pololu_error_string(device_count));
        return -1;
    }
}

//---------------------------------------------------------------
// int verifyMagSensor(pList *p)
//---------------------------------------------------------------
int verifyMagSensor(pList *p)
{
    fprintf(stdout, "\nVerifying Magnetometer Status & Version...\n");

    if(i2c_pololu_is_connected(p->adapter))
    {
        fprintf(stdout, "  Connection to: %s is OK!\n", p->portpath);
    }
    else
    {
        return 0;
    }

    i2c_pololu_clear_bus(p->adapter);

    uint8_t buf[2] = {0};
    uint8_t addr = 0x23; // example device
    uint8_t reg  = 0x36; // example register
    int rv = -1;

    // Write register index
    rv = i2c_pololu_write_to(p->adapter, addr, reg, "", 1);
    if (rv < 0)
    {
        fprintf(stdout, "  Write failed: %s\n", i2c_pololu_error_string(-rv));
        goto done;
    }
    // Read back 2 bytes
    //    uint8_t buf[2] = {0};
    rv = i2c_pololu_read_from(p->adapter, addr, reg, buf, 2);
    if (rv < 0)
    {
        fprintf(stdout, "  Read failed: %s\n", i2c_pololu_error_string(-rv));
        goto done;
    }
    //fprintf(stderr,"  Data: %02X %02X\n", buf[0], buf[1]);
    rv = buf[0];
    if(rv == (uint8_t) RM3100_VER_EXPECTED)
    {
        fprintf(stdout, "  Version is OK!: 0x%2X\n", rv);
        return 0;
    }
    else
    {
        fprintf(stdout, "  Version does NOT match!\n");
        return rv;
    }
done:
    return 1;
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
        if((rv = i2c_writebyte_mag(p, RM3100_MAG_POLL, "", 1)) != 0)
        {
            showErrorMsg(rv);
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "    [Child]: initMagSensor(POLL) != OK\n");
            fflush(OUTPUT_PRINT);
#endif
            return FALSE;
        }
    }
    else
    {
        if((rv = i2c_writebyte_mag(p, RM3100I2C_CMM, "", 1)) != 0)
        {
            showErrorMsg(rv);
#if(__DEBUG)
            fprintf(OUTPUT_PRINT, "    [Child]: initMagSensor(CMM) != OK\n");
            fflush(OUTPUT_PRINT);
#endif
            return FALSE;
        }
    }
    return rv;
}

//---------------------------------------------------------------
// int initTempSensors(volatile pList *p)
//---------------------------------------------------------------
int initTempSensors(pList *p)
{
    int rv = 0;
    // Temp sensor doesn't need any iniutialization currently.
    return rv;
}

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
