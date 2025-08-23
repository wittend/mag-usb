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
#include "main.h"

#include <linux/limits.h>

#include "i2c.h"
#include "i2c_pololu.h"
#include "cmdmgr.h"
#include "magdata.h"

//------------------------------------------
// Debugging output
//------------------------------------------
// see main.h
//
//#define _DEBUG          FALSE
//#define CONSOLE_OUTPUT  TRUE
//#define USE_WAITFOREDGE TRUE

//------------------------------------------
// Macros
//------------------------------------------
#define PPS_GPIO_PIN    27
#define PPS_TIMEOUTSECS 2.0

//------------------------------------------
// Static and Global variables
//------------------------------------------
int volatile PPS_Flag   = 0;
int volatile killflag   = 0;
char Version[32]        = MAGDATA_VERSION;
static char outBuf[256] = "";
char portpath[PATH_MAX] =  "/dev/ttyACM2";
//const char *port_name = "/dev/ttyACM2";

char fifoCtrl[] = "/home/pi/PSWS/Sstat/magctl.fifo";
char fifoData[] = "/home/pi/PSWS/Sstat/magdata.fifo";
char fifoHome[] = "/run/user/";
// char fifoCtrl[PATH_MAX] = "";
// char fifoData[PATH_MAX] = "";
// sprintf(fifoCtrl, "%s%d%s", fifoHome, getuid(), "/magctl.fifo");
// sprintf(fifoData, "%s%d%s", fifoHome, getuid(), "/magdata.fifo");
int PIPEIN  = -1;
int PIPEOUT = -1;

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
//  main()
//---------------------------------------------------------------
int main(int argc, char** argv)
{
    pList   ctl;
    pList   *p = &ctl;
    int     rv = 0;
    FILE    *outfp = (FILE *)stdout;
    char    utcStr[UTCBUFLEN] = "";
    struct  tm *utcTime = getUTC();

#if(_DEBUG)
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
    if(p != NULL)
    {
        memset(p, 0, sizeof(pList));
    }

    p->portpath         = portpath;
    p->adapter          = 0;
    p->ppsHandle        = 0;
    p->magHandle        = 0;
    p->localTempHandle  = 0;
    p->remoteTempHandle = 0;
    p->doBistMask       = 0;
    p->cc_x             = CC_400;
    p->cc_y             = CC_400;
    p->cc_z             = CC_400;
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
    p->i2cBusNumber     = RASPI_I2C_BUS1;
    p->remoteTempAddr   = MCP9808_RMT_I2CADDR_DEFAULT;
    p->magnetometerAddr = RM3100_I2C_ADDRESS;
    p->usePipes         = USE_PIPES;
    p->pipeInPath       = fifoCtrl;
    p->pipeOutPath      = fifoData;
    p->readBackCCRegs   = FALSE;

//    if((rv = getCommandLine(argc, argv, p)) != 0)
//    {
//        return rv;
//    }

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
    
#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "    [CHILD] Before setting up GPIO.\n");
    fflush(OUTPUT_PRINT);
#endif

    i2c_init(p);
    i2c_open(p, portpath);

    //-----------------------------------------
    //  Verify the Mag sensor presence and Version.
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

    //-----------------------------------------
    //  Initialize the Temp sensor registers.
    //-----------------------------------------
    if(initTempSensors(p))
    {
        utcTime = getUTC();
        strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
        fprintf(OUTPUT_ERROR, "    [CHILD] {ts: \"%s\", lastStatus: \"Unable to initialize the temperature sensor.\"}", utcStr); 
        fflush(OUTPUT_ERROR);
        exit(1);
    }

#if _DEBUG
    fprintf(OUTPUT_PRINT, "    [CHILD] Before setting up callback to: onEdge() for PPS...\n");
    fflush(OUTPUT_PRINT);
#endif
    
    //-----------------------------------------
    //  Main program loop.
    //-----------------------------------------
    while(1)
    {
        if(PPS_Flag)
        { 
            PPS_Flag = 0;
            formatOutput(p, outBuf);
            fflush(outfp);
        }
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
#if _DEBUG
        else
        {
            fputs(".", OUTPUT_PRINT);
            fflush(OUTPUT_PRINT);
        }
#endif
    }

    //-----------------------------------------
    //  Cleanup Callback, PIGPIO, and exit.
    //-----------------------------------------
#if(USE_RGPIO || USE_LGPIO)
    rv = callback_cancel(p->edge_cb_id);
#elif (USE_PIGPIO)  
    rv = event_callback_cancel(p->edge_cb_id);
#endif
    //termGPIO(p);
    exit(rv);
}

//---------------------------------------------------------------
// void formatOutput(volatile pList *p, char *outBuf)
//---------------------------------------------------------------
char *formatOutput(pList *p, char *outBuf)
{
    char fmtBuf[200] ="";
    int fmtBuf_len = sizeof fmtBuf;
    struct tm *utcTime = getUTC();
    char utcStr[128] ="";
    double xyz[3];
    int localTemp = 0;
    int remoteTemp = 0;
    float rcLocalTemp = 0.0;
    float rcRemoteTemp = 0.0;

    strncpy(outBuf, "", 1);

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "\n    [Child]: formatOutput()...\n");
    fflush(OUTPUT_PRINT);
#endif

    localTemp = readLocalTemp(p);
    rcLocalTemp = localTemp * 0.0625;

    remoteTemp = readRemoteTemp(p);
    rcRemoteTemp = remoteTemp * 0.0625;

    readMagPOLL(p);

//    xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain) * 1000; // make microTeslas -> nanoTeslas
//    xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain) * 1000; // make microTeslas -> nanoTeslas
//    xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain) * 1000; // make microTeslas -> nanoTeslas

    xyz[0] = (((double)p->XYZ[0] / p->NOSRegValue) / p->x_gain);
    xyz[1] = (((double)p->XYZ[1] / p->NOSRegValue) / p->y_gain);
    xyz[2] = (((double)p->XYZ[2] / p->NOSRegValue) / p->z_gain);

    snprintf(fmtBuf, fmtBuf_len, "{ ");
    strncat(outBuf, fmtBuf, strlen(fmtBuf));

    utcTime = getUTC();

#if(FOR_GRAPE2)
    strftime(utcStr, UTCBUFLEN, "%Y%m%e%y%M%S", utcTime);              // YYYYMMDDHHMMSS  (Gaak!)
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":%s", utcStr);
#else
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);                // RFC 2822: "%a, %d %b %Y %T %z"
    snprintf(fmtBuf, fmtBuf_len, "\"ts\":\"%s\"", utcStr);
#endif

    strncat(outBuf, fmtBuf, strlen(fmtBuf));

    if(rcLocalTemp < -100.0)
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"lt\":0.0");
        strncat(outBuf, fmtBuf, strlen(fmtBuf));
    }
    else
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"lt\":%.2f",  rcLocalTemp);
        strncat(outBuf, fmtBuf, strlen(fmtBuf));
    }

    if(rcRemoteTemp < -100.0)
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"rt\":0.0");
        strncat(outBuf, fmtBuf, strlen(fmtBuf));
    }
    else
    {
        snprintf(fmtBuf, fmtBuf_len, ", \"rt\":%.2f",  rcRemoteTemp);
        strncat(outBuf, fmtBuf, strlen(fmtBuf));
    }

    snprintf(fmtBuf, fmtBuf_len, ", \"x\":%.3f", xyz[0]);
    strncat(outBuf, fmtBuf, strlen(fmtBuf));
    snprintf(fmtBuf, fmtBuf_len, ", \"y\":%.3f", xyz[1]);
    strncat(outBuf, fmtBuf, strlen(fmtBuf));
    snprintf(fmtBuf, fmtBuf_len, ", \"z\":%.3f", xyz[2]);
    strncat(outBuf, fmtBuf, strlen(fmtBuf));

    snprintf(fmtBuf, fmtBuf_len, " }\n");
    strncat(outBuf, fmtBuf, strlen(fmtBuf));

#if(CONSOLE_OUTPUT)
    fprintf(OUTPUT_PRINT, "    [CHILD]: %s", outBuf);
    fflush(OUTPUT_PRINT);
#elif(USE_PIPES)
    write(PIPEOUT, outBuf);
#else    
    fprintf(OUTPUT_PRINT, "    [CHILD]: %s", outBuf);
    fflush(OUTPUT_PRINT);
#endif

    return outBuf;
}

// #if(USE_POLOLU)
// #define i2c_read_i2c_block_data     i2c_readbuf
// #define i2c_write_byte_data         i2c_writebuf
// #endif
//
//------------------------------------------
// readLocalTemp(volatile pList *p)
//------------------------------------------
int readLocalTemp(pList *p)
{
    int temp = -9999;
    char data[2] = {0};

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "[Child]: readLocalTemp()...\n");
    fflush(OUTPUT_PRINT);
#endif

    //if((temp = pololu_i2c_read_from((pololu_i2c_adapter *)p->po, p->localTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
    if((temp = i2c_read(p, MCP9808_REG_AMBIENT_TEMP) <= 0))
    {
        fprintf(OUTPUT_ERROR, "Error : I/O error reading temp sensor at address: [0x%2X].\n", MCP9808_REG_AMBIENT_TEMP);
        showErrorMsg(temp);
    }
    else
    {
        // Convert the data to 13-bits
        temp = ((data[0] & 0x1F) * 256 + data[1]);
        if(temp > 4095)
        {
            temp -= 8192;
        }
    }
    return temp;
}

//------------------------------------------
// readRemoteTemp(volatile pList *p)
//------------------------------------------
int readRemoteTemp(pList *p)
{
    int temp = -9999;
    char data[2] = {0};

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "[Child]: readRemoteTemp()...\n");
    fflush(OUTPUT_PRINT);
#endif

//    if((temp = pololu_i2c_read_from((pololu_i2c_adapter *)adapter, p->localTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
    if((temp = i2c_readbuf(p->remoteTempHandle, MCP9808_REG_AMBIENT_TEMP, data, 2) <= 0))
    {
        fprintf(OUTPUT_ERROR, "Error : I/O error reading temp sensor at address: [0x%2X].\n", MCP9808_REG_AMBIENT_TEMP);
        showErrorMsg(temp);
    }
    else
    {
        // Convert the data to 13-bits
        temp = ((data[0] & 0x1F) * 256 + data[1]);
        if(temp > 4095)
        {
            temp -= 8192;
        }
    }
    return temp;
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

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: readMagPOLL()...\n");
    fflush(OUTPUT_PRINT);
#endif

    // Write command to  use Polled measurement Mode.
    rv = i2c_read(p, MCP9808_REG_AMBIENT_TEMP);
    if(rv != 0)
    {
        showErrorMsg(rv);
#if(_DEBUG)
        fprintf(OUTPUT_PRINT, "    [Child]: Write POLL mode < 0.\n");
        fflush(OUTPUT_PRINT);
#endif
        usleep(p->DRDYdelay);
    }
#if(_DEBUG)
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

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: Before Write to RM3100I2C_XYZ.\n");
    fflush(OUTPUT_PRINT);
#endif

    // Tell the sensor that you want to read XYZ data. RM3100I2C_POLLXYZ
    // rv = i2c_write_byte_data(p->pi, p->magHandle, RM3100I2C_XYZ, TRUE);
    rv = i2c_write(p, p->magHandle,RM3100I2C_XYZ);
//    rv = pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t 1 );
    rv = i2c_readbuf(p, RM3100I2C_XYZ, xyzBuf, 3 );
    if(rv < 0)
    {
        showErrorMsg(p->magHandle);
    }
    else if(rv == 0)
    {
        // Wait for DReady Flag.
        //rv = i2c_readbyte(p, p->magHandle);
        rv = i2c_read(p, p->magHandle);
        rv = (rv & RM3100I2C_READMASK);
        while((rv != RM3100I2C_READMASK))
        {
    //        rv = i2c_readbyte(p, p->magHandle);
            rv = i2c_read(p, p->magHandle);
            rv = (rv & RM3100I2C_READMASK);
        }

#if(_DEBUG)
        fprintf(OUTPUT_PRINT, "    [Child]: Before Read RM3100I2C_XYZ. rv: %u\n", rv);
        fflush(OUTPUT_PRINT);
#endif

        // Read the data registers.
        //rv = i2c_read_device(p->pi, p->magHandle, xyzBuf, XYZ_BUFLEN);
        //rv = i2c_read_i2c_block_data(p->pi, p->magHandle, RM3100I2C_XYZ, (char *)xyzBuf, XYZ_BUFLEN);
        //rv = pololu_i2c_write_to( pololu_i2c_adapter *adapter, RM3100I2C_XYZ, (char *)xyzBuf, XYZ_BUFLEN);
        rv = i2c_readbuf(p, RM3100I2C_XYZ, xyzBuf, XYZ_BUFLEN);
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
#if(_DEBUG)
            fprintf(OUTPUT_PRINT, "\n    [Child]: readMagPOLL() -  Bytesread: %u.\n", bytes_read);
//            fprintf(OUTPUT_PRINT, "p->XYZ[0] : %.3X, p->XYZ[1]: %.3X, p->XYZ[2]: %.3X.\n", p->XYZ[0], p->XYZ[1], p->XYZ[2]);
            fflush(OUTPUT_PRINT);
#endif
        }
        else
        {
            showErrorMsg(rv);
#if(_DEBUG)
            fprintf(OUTPUT_PRINT, "\n    [Child]: readMagPOLL() -  Bytesread: %u.\n", bytes_read);
 //           fprintf(OUTPUT_PRINT, "p->XYZ[0] : %.3X, p->XYZ[1]: %.3X, p->XYZ[2]: %.3X.\n", p->XYZ[0], p->XYZ[1], p->XYZ[2]);
            fflush(OUTPUT_PRINT);
#endif
        }
    }
    return bytes_read;
}

//---------------------------------------------------------------
// void verifyMagSensor(volatile pList *p)
//---------------------------------------------------------------
int verifyMagSensor(pList *p)
{
    int rv = 0;
    char revBuf[] = {0};

#if(_DEBUG)
    fprintf(OUTPUT_PRINT,"    [Child]: In VerifyMagSensor()...\n");
    fflush(OUTPUT_PRINT);
#endif
    
    //-----------------------------------------
    // Make sure PGPIO connection is OK.
    //-----------------------------------------
    if(p >= 0)
    {
        p->magRevId = 0;

        //-----------------------------------------
        // Read the version register.
        //-----------------------------------------
        if((rv = i2c_readbuf(p, p->magHandle, RM3100I2C_REVID, revBuf, 1)) > 0)
        {
            p->magRevId = *revBuf;
            if(*revBuf != (uint8_t) RM3100_VER_EXPECTED)
            {
                // Fail, exit...
                fprintf(OUTPUT_ERROR, "\n    [Child]: RM3100 REVID NOT CORRECT: ");
                fprintf(OUTPUT_ERROR, "RM3100 REVID: 0x%X <> EXPECTED: 0x%X.\n\n", (unsigned) p->magRevId, RM3100_VER_EXPECTED);
                fflush(OUTPUT_ERROR);
                return 1;
            }
            else
            {
//#if(_DEBUG)
//                fprintf(OUTPUT_PRINT,"    [Child]: RM3100 Detected Properly: ");
//                fprintf(OUTPUT_PRINT,"REVID: %x.\n", p->magRevId);
//                fflush(OUTPUT_PRINT);
//#endif
                return 0;
            }
        }
        else
        {
            showErrorMsg(rv);
#if(_DEBUG)
            fprintf(OUTPUT_PRINT,"    [Child]: Error i2c_read_i2c_block_data returned: %u.\n", rv);
            fflush(OUTPUT_PRINT);
#endif
            return(1);
        }
    }
    return 0;
}

//------------------------------------------
// setNOSReg(volatile pList *p)
//------------------------------------------
int setNOSReg(pList *p)
{
    int rv = 0;
//#if _DEBUG
//    fprintf(OUTPUT_PRINT, "    [Child]: In setNOSReg():: Setting undocumented NOS register to value: %2X\n", p->NOSRegValue);
//#endif
    return rv;
}

// //---------------------------------------------------------------
// // void initGPIO(pList *p)
// //---------------------------------------------------------------
// int initGPIO(volatile pList *p)// {
//
// #if _DEBUG
//     fprintf(OUTPUT_PRINT, "    [CHILD]: In initGPIO(pList *p) before: pigpio_start()...\n");
//     fflush(OUTPUT_PRINT);
// #endif
//
// #if( USE_RGPIO || USE_LGPIO || USE_PIGPIO_IF2)
//     //-----------------------------------------
//     // Try to connect to pigpio daemon.
//     //-----------------------------------------
//     #if(USE_RGPIO)
//         if((p->po = rgpiod_start(NULL, NULL)) >= 0)
//     #elif (USE_LGPIO || USE_PIGPIO_IF2)
//         if((p->po = pigpio_start(NULL, NULL)) >= 0)
//     #endif
//         {
//     #if _DEBUG
//             fprintf(OUTPUT_PRINT, "    [CHILD] pigpio_start() OK, returns handle %i...\n", p->po);
//             fflush(OUTPUT_PRINT);
//     #endif
//         }
//         else
//         {
//     #if _DEBUG
//             showErrorMsg(p->po);
//             fprintf(OUTPUT_PRINT, "    [CHILD] pigpio_start() FAIL, returns: %i...\n", p->po);
//             fflush(OUTPUT_PRINT);
// #endif
//             return -1;
//         }
// #endif
//
//
//     //-----------------------------------------
//     // Register the Magnetometer address.
//     //-----------------------------------------
//     if((p->magHandle = i2c_open(p, (unsigned) RASPI_I2C_BUS1, (unsigned) RM3100_I2C_ADDRESS, (unsigned) 0)) >= 0)
//     {
// #if _DEBUG
//         showErrorMsg(p->magHandle);
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(RM3100) OK. Handle: %i\n", p->magHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if _DEBUG
//         showErrorMsg(p->magHandle);
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(RM3100) FAIL. Handle: %i\n", p->magHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//         fflush(OUTPUT_PRINT);
//         return -1;
//     }
//
//     //-----------------------------------------
//     // Register the Local Temp Sensor address.
//     //-----------------------------------------
//     if((p->localTempHandle = i2c_open(p, (unsigned) RASPI_I2C_BUS1, (unsigned) MCP9808_LCL_I2CADDR_DEFAULT, (unsigned) 0) >= 0))
//     {
// #if _DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) OK. Handle: %i\n", p->localTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if _DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) FAIL. Handle: %i\n", p->localTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//         showErrorMsg(p->localTempHandle);
//         return -1;
//     }
//
//     //-----------------------------------------
//     // Register the Remote Temp Sensor address.
//     //-----------------------------------------
//     if((p->remoteTempHandle = i2c_open(p, (unsigned) RASPI_I2C_BUS1, (unsigned) MCP9808_RMT_I2CADDR_DEFAULT, (unsigned) 0) >= 0))
//     {
// #if _DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) OK. Handle: %i\n", p->remoteTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if _DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) FAIL. Handle: %i\n", p->remoteTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//         showErrorMsg(p->remoteTempHandle);
//         return -1;
//     }
//     return p;
// }

//---------------------------------------------------------------
// void termGPIO(volatile pList p)
//---------------------------------------------------------------
void termGPIO(pList *p)
{
    // Knock down all of the pigpio setup here.
    p->magHandle = i2c_close(p, p->magHandle);
    // p->localTempHandle = i2c_close(p->po, p->localTempHandle);
    // p->remoteTempHandle = i2c_close(p->po, p->remoteTempHandle);
    // rgpiod_stop(p->po);

#if(_DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: termGPIO(pList p)...\n");
    fflush(OUTPUT_PRINT);
#endif
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
        if((rv = i2c_writebyte(p, p->magHandle, RM3100_MAG_POLL, TRUE)) != 0)
        {
            showErrorMsg(rv);
#if(_DEBUG)
            fprintf(OUTPUT_PRINT, "    [Child]: initMagSensor(POLL) != OK\n");
            fflush(OUTPUT_PRINT);
#endif
            return FALSE;
        }
    }
    else
    {
        if((rv = i2c_writebyte(p, p->magHandle, RM3100I2C_CMM, TRUE)) != 0)
        {
            showErrorMsg(rv);
#if(_DEBUG)
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

#if(USE_PIGPIO || USE_PIGPIO_IF2)
//---------------------------------------------------------------
// void showErrorMsg(int rv)
//---------------------------------------------------------------
void showErrorMsg(int rv)
{
    char    utcStr[UTCBUFLEN] = "";
    struct  tm *utcTime = getUTC();  
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);
#if(CONSOLE_OUTPUT)
    fprintf(OUTPUT_PRINT, "    [Child]: { \"ts\": \"%s\", \"lastError\": \"%s\" }\n", utcStr, lgpio_error(rv));
    fflush(OUTPUT_PRINT);
#else
    char errstr[MAXPATHBUFLEN] = "";
    sprintf(errstr, "    [Child]: { \"ts\": \"%s\", \"lastError\": \"%s\" }\n", utcStr, lgpio_error(rv));
    write(PIPEOUT, errstr);
#endif
}
#endif

//---------------------------------------------------------------
// void onEdge(void)
//---------------------------------------------------------------
void onEdge(void)
{
#if(_DEBUG)
    fputs("|", OUTPUT_PRINT);
    fflush(OUTPUT_PRINT);
#endif
    PPS_Flag = TRUE;
}

