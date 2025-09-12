//=========================================================================
// magdata.c
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
//#include "i2c-pigpio.h"
//#include "main.h"
#include "main.h"
#include "magdata.h"
#include "i2c.h"
//#include "rm3100.h"
//#include "rm3100.h"

//------------------------------------------
// Static variables
//------------------------------------------
// static char  mSamples[9];
extern char *Version;
//#if(USE_PIPES)
//char outputPipeName[MAXPATHBUFLEN]  = "/home/web/wsroot/pipein.fifo";
//char inputPipeName[MAXPATHBUFLEN]   = "/home/web/wsroot/pipeout.fifo";
//#endif



// //---------------------------------------------------------------
// // void initGPIO(pList *p)
// //---------------------------------------------------------------
// int initGPIO(volatile pList *p)// {
//
// #if __DEBUG
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
//     #if __DEBUG
//             fprintf(OUTPUT_PRINT, "    [CHILD] pigpio_start() OK, returns handle %i...\n", p->po);
//             fflush(OUTPUT_PRINT);
//     #endif
//         }
//         else
//         {
//     #if __DEBUG
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
// #if __DEBUG
//         showErrorMsg(p->magHandle);
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(RM3100) OK. Handle: %i\n", p->magHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if __DEBUG
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
// #if __DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) OK. Handle: %i\n", p->localTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if __DEBUG
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
// #if __DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) OK. Handle: %i\n", p->remoteTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//     }
//     else
//     {
// #if __DEBUG
//         fprintf(OUTPUT_PRINT, "    [CHILD] i2c_open(MCP9808) FAIL. Handle: %i\n", p->remoteTempHandle );
//         fflush(OUTPUT_PRINT);
// #endif
//         showErrorMsg(p->remoteTempHandle);
//         return -1;
//     }
//     return p;
// }

////------------------------------------------
//// readTemp(pList *p)
////------------------------------------------
//int readTemp(pList *p)
//{
//    int temp = -9999;
//    char data[2] = {0};
//    char reg[1] = {MCP9808_REG_AMBIENT_TEMP};
//
////    if(i2c_read_device(p->pi, data, 2) != 2)
////    {
////        fprintf(stderr, "Error : I/O error reading temp sensor at address: [0x%2X].\n", devAddr);
////    }
////    else
////    {
////        // Convert the data to 13-bits
////        temp = ((data[0] & 0x1F) * 256 + data[1]);
////        if(temp > 4095)
////        {
////            temp -= 8192;
////        }
////    }
//    return temp;
//}

////------------------------------------------
//// readMagCMM()
////------------------------------------------
//int readMagCMM(pList *p, int32_t *XYZ)
//{
//    int rv = 0;
//    int bytes_read = 0;
//
//    // i2c_setAddress(p->pi, devAddr);
//    // Check if DRDY went high and wait unit high before reading results
////    while((rv = (i2c_read(p->pi, RM3100I2C_STATUS)) & RM3100I2C_READMASK) != RM3100I2C_READMASK)
////    {
////    }
//    // Read the XYZ registers
////    if((bytes_read = i2c_readbuf(p->pi, RM3100I2C_XYZ, (unsigned char*) &mSamples, sizeof(mSamples)/sizeof(char))) != sizeof(mSamples)/sizeof(char))
////    {
////        perror("i2c transaction i2c_readbuf() failed.\n");
////    }
//    XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
//    XYZ[0] |= mSamples[1] * 256;
//    XYZ[0] |= mSamples[2];
//
//    XYZ[1] = ((signed char)mSamples[3]) * 256 * 256;
//    XYZ[1] |= mSamples[4] * 256;
//    XYZ[1] |= mSamples[5];
//
//    XYZ[2] = ((signed char)mSamples[6]) * 256 * 256;
//    XYZ[2] |= mSamples[7] * 256;
//    XYZ[2] |= mSamples[8];
//
//    return bytes_read;
//}

////------------------------------------------
//// readMagPOLL()
////------------------------------------------
//int readMagPOLL(pList *p, int32_t *XYZ)
//{
//    int rv = 0;
//    int bytes_read = 0;
//    short pmMode = (PMMODE_ALL);
//
////    i2c_setAddress(p->pi, devAddr);
////    // Write command to  use Continuous measurement Mode.
////    i2c_write(p->pi, RM3100_MAG_POLL, pmMode);
//    // if a delay is specified after DRDY goes high, sleep it off.
//    if(p->DRDYdelay)
//    {
//        usleep(p->DRDYdelay);
//    }
//    // Check if DRDY went high and wait unit high before reading results
////    while((rv = (i2c_read(p->pi, RM3100I2C_STATUS)) & RM3100I2C_READMASK) != RM3100I2C_READMASK)
////    {
////    }
//    // Read the XYZ registers
////    if((bytes_read = i2c_readbuf(p->pi, RM3100I2C_XYZ, (unsigned char*) &mSamples, sizeof(mSamples)/sizeof(char))) != sizeof(mSamples)/sizeof(char))
////    {
////        perror("i2c transaction i2c_readbuf() failed.\n");
////    }
//    XYZ[0] = ((signed char)mSamples[0]) * 256 * 256;
//    XYZ[0] |= mSamples[1] * 256;
//    XYZ[0] |= mSamples[2];
//
//    XYZ[1] = ((signed char)mSamples[3]) * 256 * 256;
//    XYZ[1] |= mSamples[4] * 256;
//    XYZ[1] |= mSamples[5];
//
//    XYZ[2] = ((signed char)mSamples[6]) * 256 * 256;
//    XYZ[2] |= mSamples[7] * 256;
//    XYZ[2] |= mSamples[8];
//
//    return bytes_read;
//}


////------------------------------------------
//// openI2CBus()
////------------------------------------------
//int openI2CBus(pList *p)
//{
//    p->pi = -1;
//    
//    char pathStr[64] = "";
//    snprintf(pathStr, sizeof(pathStr), "/dev/i2c-%i", p->i2cBusNumber);
//    
//    if((p->pi = open(pathStr, O_RDWR)) < 0)
//    {
//        perror("Bus open failed\n");
//        return -1;
//    }
//    return p->pi;
//}

////--------------------------------------------------------------------
//// closeI2CBus()
////
//// Close I2C bus
////--------------------------------------------------------------------
//void closeI2CBus(int pi)
//{
//    close(pi);
//}

////------------------------------------------
//// setNOSReg()
////------------------------------------------
//int setNOSReg(pList *p)
//{
//    int rv;
//    printf("\nIn setNOSReg():: Setting undocumented NOS register to value: %2X\n", p->NOSRegValue);
////    rv = i2c_write(p->pi, RM3100I2C_NOS, p->NOSRegValue);
//    return rv;
//}

//------------------------------------------
// setMagSampleRate()
//------------------------------------------
unsigned short setMagSampleRate(pList *p, unsigned short sample_rate)
{
    int i;
    const unsigned short int supported_rates[][2] = 
    {
        /* [Hz], register value */
        {   2,  0x0A},   // up to 2Hz
        {   4,  0x09},   // up to 4Hz
        {   8,  0x08},   // up to 8Hz
        {  16,  0x07},   // up to 16Hz
        {  31,  0x06},   // up to 31Hz
        {  62,  0x05},   // up to 62Hz
        { 125,  0x04},   // up to 125Hz
        { 220,  0x03}    // up to 250Hz
    };
    for(i = 0; i < sizeof(supported_rates)/(sizeof(unsigned short int) * 2) - 1; i++)
    {
        if(sample_rate <= supported_rates[i][0])
        {
            break;
        }
    }
    p->CMMSampleRate = supported_rates[i][0];
    // i2c_write(p->pi, RM3100I2C_TMRC, p->CMMSampleRate);
    return p->CMMSampleRate;
}

//------------------------------------------
// getMagSampleRate();
// The actual sample rate of the sensor.
//------------------------------------------
unsigned short getMagSampleRate(pList *p)
{
    return p->CMMSampleRate;
}

//
////------------------------------------------
//// getMagRev(pList *p)
////------------------------------------------
//int getMagRev(pList *p)
//{
//    // Set address of the RM3100
////    i2c_setAddress(p->pi,  p->magnetometerAddr);
//
//    // Check Version
////    if((p->magRevId = i2c_read(p->pi, RM3100I2C_REVID)) != (uint8_t)RM3100_VER_EXPECTED)
////    {
////        // Fail, exit...
////        fprintf(stderr, "\nRM3100 REVID NOT CORRECT: ");
////        fprintf(stderr, "RM3100 REVID: 0x%X <> EXPECTED: 0x%X.\n\n", p->magRevId, RM3100_VER_EXPECTED);
////        fflush(stdout);
////        return 0;
////    }
////    else
////    {
////        fprintf(stdout,"RM3100 Detected Properly: ");
////        fprintf(stdout,"REVID: %x.\n", p->magRevId);
////    }
//    return p->magRevId;
//}

////------------------------------------------
//// setup_mag()
////------------------------------------------
//int setup_mag(pList *p)
//{
//    int rv = SensorOK;
//
//    // Set address of the RM3100
////    i2c_setAddress(p->pi,  p->magnetometerAddr);
//    // Check Version
//    if(!getMagRev(p))
//    {
//        exit (1);
//    }
//    // Setup the NOS register
//    // setNOSReg(p);
//    // Clear out these registers
////    i2c_write(p->pi, RM3100_MAG_POLL, 0);
////    i2c_write(p->pi, RM3100I2C_CMM,  0);
//    // Initialize CC settings
//    setCycleCountRegs(p);
//    // Sleep for 1 second
//    usleep(100000);                           // delay to help monitor DRDY pin on eval board
//    return rv;
//}

//---------------------------------------------------------------
// void termGPIO(volatile pList p)
//---------------------------------------------------------------
void termGPIO(pList *p)
{
    // Close the adaptor here.
    i2c_close(p);
    // p->localTempHandle = i2c_close(p->po, p->localTempHandle);
    // p->remoteTempHandle = i2c_close(p->po, p->remoteTempHandle);
    // rgpiod_stop(p->po);

#if(__DEBUG)
    fprintf(OUTPUT_PRINT, "    [Child]: termGPIO(pList p)...\n");
    fflush(OUTPUT_PRINT);
#endif
}

//---------------------------------------------------------------
// void showErrorMsg(int rv)
//---------------------------------------------------------------
void showErrorMsg(int rv)
{
    char    utcStr[UTCBUFLEN] = "";
    struct  tm *utcTime = getUTC();
    strftime(utcStr, UTCBUFLEN, "%d %b %Y %T", utcTime);

#if(USE_PIGPIO || USE_PIGPIO_IF2)
    #if(CONSOLE_OUTPUT)
        fprintf(OUTPUT_PRINT, "    [Child]: { \"ts\": \"%s\", \"lastError\": \"%s\" }\n", utcStr, lgpio_error(rv));
        fflush(OUTPUT_PRINT);
    #else
        char errstr[MAXPATHBUFLEN] = "";
        sprintf(errstr, "    [Child]: { \"ts\": \"%s\", \"lastError\": \"%s\" }\n", utcStr, lgpio_error(rv));
        write(PIPEOUT, errstr);
    #endif
#else
    fprintf(OUTPUT_PRINT, "    [Child]: { \"ts\": \"%s\", \"lastError\": \"%s\" }\n", utcStr,  pololu_i2c_error_string(rv));
    fflush(OUTPUT_PRINT);
#endif
}
//------------------------------------------
// setNOSReg(volatile pList *p)
//------------------------------------------
int setNOSReg(pList *p)
{
    int rv = 0;
    //#if __DEBUG
    //    fprintf(OUTPUT_PRINT, "    [Child]: In setNOSReg():: Setting undocumented NOS register to value: %2X\n", p->NOSRegValue);
    //#endif
    return rv;
}

//------------------------------------------
// runBIST()
// Runs the Built In Self Test.
//------------------------------------------
int runBIST(pList *p)
{
    return 0;
    //return i2c_read(p->pi, RM3100I2C_TMRC);
}

//------------------------------------------
// startCMM()
// Starts Continuous Measurement Mode
//------------------------------------------
int startCMM(pList *p)
{
    int rv = 0;
//    short cmmMode = (CMMMODE_ALL);   // 71 d
//    rv = i2c_write(p->pi, RM3100I2C_CMM, cmmMode);
    return rv;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Dave,
//
// Here is a general equation for gain taken directly from correspondence with PNI which I use in my Python scripts.
// Gn=(Aval*(0.3671*Cycnt+1.5)/1000)
// (0.3671*cycle count + 1.5)  when divided into the X Y or Z result with no averaging gives the correct value in micro teslas
// Aval/1000 times (0.3671*cycle count + 1.5) when divided into the X Y or Z result gives the correct value in nano teslas.
// Conversely, you can multiply the X Y or Z values by  1000/(Aval*(0.3671*Cycnt+1.5))
// As far as I know, it is an exact gain equation for the RM3100 and works for ANY cycle count .... like 375, 405, 125, etc, etc.  No error prone lookup tables.
//
// Jules
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//------------------------------------------
// getCCGainEquiv()
//   Gn=(Aval*(0.3671*Cycnt+1.5)/1000)
//------------------------------------------
unsigned short getCCGainEquiv(unsigned short CCVal)
{
    unsigned short gain = 0;
    double dGain = (0.3671 * CCVal + 1.5); 
    gain = (unsigned short) dGain;
    return gain;
}

//------------------------------------------
// setCycleCountRegs()
//------------------------------------------
void setCycleCountRegs(pList *p)
{
    //int i = 0;
//    i2c_write(p->pi, RM3100I2C_CCX_1, (p->cc_x >> 8));
//    i2c_write(p->pi, RM3100I2C_CCX_0, (p->cc_x & 0xff));
    p->x_gain = getCCGainEquiv(p->cc_x);
//    i2c_write(p->pi, RM3100I2C_CCY_1, (p->cc_y >> 8));
//    i2c_write(p->pi, RM3100I2C_CCY_0, (p->cc_y & 0xff));
    p->y_gain = getCCGainEquiv(p->cc_y);
//    i2c_write(p->pi, RM3100I2C_CCZ_1, (p->cc_y >> 8));
//    i2c_write(p->pi, RM3100I2C_CCZ_0, (p->cc_y & 0xff));
    p->z_gain = getCCGainEquiv(p->cc_z);
    // Write NOSRegValue to  register 0A
//    i2c_write(p->pi, RM3100I2C_NOS,   (uint8_t)(p->NOSRegValue));

//        fprintf(stderr, "\nIn setCycleCountRegs():: Setting NOS register to value: %2X\n", p->NOSRegValue);
//        fprintf(stderr, "CycleCounts  - X: %u, Y: %u, Z: %u.\n", p->cc_x, p->cc_y, p->cc_x);
//        fprintf(stderr, "Gains        - X: %u, Y: %u, Z: %u.\n", p->x_gain, p->y_gain, p->z_gain);
//        fprintf(stderr, "NOS Register - %2X.\n", p->NOSRegValue);
}

//------------------------------------------
// readCycleCountRegs()
//------------------------------------------
void readCycleCountRegs(pList *p)
{
    uint8_t regCC[7]= { 0, 0, 0, 0, 0, 0, 0 };

//    i2c_setAddress(p->pi, p->magnetometerAddr);
    //  Read register settings
//    i2c_readbuf(p->pi, RM3100I2C_CCX_1, regCC, 7);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    0, (uint8_t)regCC[0]);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    1, (uint8_t)regCC[1]);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    2, (uint8_t)regCC[2]);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    3, (uint8_t)regCC[3]);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    4, (uint8_t)regCC[4]);
    fprintf(stdout, "regCC[%i]: 0x%X\n",    5, (uint8_t)regCC[5]);
    fprintf(stdout, "regCC[%i]: 0x%X\n\n",  6, (uint8_t)regCC[6]);
}

