//=========================================================================
// i2c_pololu.c
//
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// License:     GPL 3.0
// Note:        1. Notes and tips
//              - Keep the app code free of #ip->adapter.fdefs. All conditional logic
//                  lives inside backend implementations.
//
//              - If a backend requires a different open model (e.g., it
//                  needs bus and address together), adapt your i2c_
//                  open/i2c_setAddress implementation accordingly. For
//                  example, treat i2c_open as a no-op and implement actual
//                  open in i2c_setAddress.
//
//              - If any function in i2c.h has typos or unclear semantics,
//                  consider correcting them across all backends at once
//                  for consistency before growing the project.
//
//              - If you prefer one “unified” target name, you can also add
//                  an option(I2C_BACKEND "..." "linux") and generate a
//                  single rm3100 executable by selecting the backend
//                  with -DI2C_BACKEND=pigpio at configure time. But since
//                  you asked for make , multiple targets are the simplest.
//
//      This approach scales well: add a new backend by dropping in a new
//      i2.c and adding one library + one executable in CMake.
//
//  *BUT*:  I neet to go back and mak all the code use these, or eliminate
//  them altogether.  Currently there is a NASTY mix.
//=========================================================================
// backends/i2c_pololu.c
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"
#include "i2c.h"
#include "i2c-pololu.h" // your Pololu API headers
#include "rm3100.h"


//------------------------------------------
// readMagPOLL()
//------------------------------------------
int i2c_readMagPOLL(pList *p)
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
// i2c_init(pList *p)
// This is almost as pointless as i2c_pololu_init() which it calls.
//---------------------------------------------------------------
int i2c_init(pList *p)
{
    return i2c_pololu_init(p->adapter);
}

//---------------------------------------------------------------
// i2c_open(pList *p, const char *portpath)
//---------------------------------------------------------------
int i2c_open(pList *p, const char *portpath)
{
    // Map to Pololu open/init sequence
    struct stat sb;
    if(!stat(p->portpath, &sb))
    {
        return i2c_pololu_connect(p->adapter, p->portpath);
    }
    else
    {
        char errstr[1024] = "";
        sprintf(errstr, "Device %s does not exist or is in use. Exiting...", p->portpath);
        perror(errstr);
        // exit(EXIT_FAILURE);
        return -1;
    }
}

//---------------------------------------------------------------
// i2c_setAddress()
//---------------------------------------------------------------
void i2c_setAddress(pList *p, int devAddr)
{
//    p->adapter.fd; (void)devAddr;
}

//---------------------------------------------------------------
// i2c_setBitRate()
//---------------------------------------------------------------
void i2c_setBitRate(pList *p, int devspeed)
{
//    p->adapter.fd; (void)devspeed;
}

//-----------------------------------------------------------------------------
// Calls for the Temperature sensor.
//-----------------------------------------------------------------------------

//---------------------------------------------------------------
// i2c_write_temp()
//---------------------------------------------------------------
int i2c_write_temp(pList *p, uint8_t reg, uint8_t value)
{
    int rv;
    rv = i2c_pololu_write_to(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, &value, (uint8_t) 1);
    return rv;
}

//---------------------------------------------------------------
// i2c_read_temp()
//---------------------------------------------------------------
uint8_t i2c_read_temp(pList *p, uint8_t reg)
{
    uint8_t  rv;
    i2c_pololu_read_from(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) &rv, (uint8_t) 1 );
    return rv;
}

//---------------------------------------------------------------
// i2c_writebyte_temp()
//---------------------------------------------------------------
int i2c_writebyte_temp(pList *p, uint8_t reg, char* buffer, short int length)
{
    return i2c_pololu_write_to(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) 1);
}

//---------------------------------------------------------------
// i2c_reabyte_temp()
//---------------------------------------------------------------
int i2c_reabyte_temp(pList *p, uint8_t reg, uint8_t* buf, short int length)
{
    return i2c_pololu_read_from(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) 1);
}

//---------------------------------------------------------------
// i2c_writebuf_temp()
//---------------------------------------------------------------
int i2c_writebuf_temp(pList *p, uint8_t reg, char* buf, short int length)
{
    return i2c_pololu_write_to(p->adapter,(uint8_t) p->remoteTempAddr, (uint8_t) reg, buf, (uint8_t) length);
}

//---------------------------------------------------------------
// i2c_readbuf_temp()
//---------------------------------------------------------------
int i2c_readbuf_temp(pList *p, uint8_t reg, uint8_t *buf, uint8_t length)
{
    return i2c_pololu_read_from(p->adapter, p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) length );
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


//-----------------------------------------------------------------------------
// Calls for the Magnetometer
//-----------------------------------------------------------------------------

//---------------------------------------------------------------
// i2c_write_mag()
//---------------------------------------------------------------
int i2c_write_mag(pList *p, uint8_t reg, uint8_t value)
{
    int rv = 0;
    rv = i2c_pololu_write_to(p->adapter, (uint8_t)p->magAddr, (uint8_t) reg, &value, (uint8_t) 1);
    return rv;
}

//---------------------------------------------------------------
// i2c_read_mag()
//---------------------------------------------------------------
uint8_t i2c_read_mag(pList *p, uint8_t reg)
{
    uint8_t  rv;
    i2c_pololu_read_from(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) &rv, (uint8_t) 1 );
    return rv;
}

//---------------------------------------------------------------
// i2c_writebyte_mag()
//---------------------------------------------------------------
int i2c_writebyte_mag(pList *p, uint8_t reg, char* buffer, short int length)
{
    return i2c_pololu_write_to(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) 1);
}

//---------------------------------------------------------------
// i2c_reabyte_mag()
//---------------------------------------------------------------
int i2c_reabyte_mag(pList *p, uint8_t reg, uint8_t* buf, short int length)
{
    return i2c_pololu_read_from(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) 1);
}

//---------------------------------------------------------------
// i2c_writebuf_mag()
//---------------------------------------------------------------
int i2c_writebuf_mag(pList *p, uint8_t reg, char* buf, short int length)
{
    return i2c_pololu_write_to(p->adapter,(uint8_t) p->magAddr, (uint8_t) reg, buf, (uint8_t) length);
}

//---------------------------------------------------------------
// i2c_readbuf_mag()
//---------------------------------------------------------------
int i2c_readbuf_mag(pList *p, uint8_t reg, uint8_t *buf, uint8_t length)
{
    return i2c_pololu_read_from(p->adapter, p->magAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) length );
}

//---------------------------------------------------------------
// int initMagSensor(volatile pList *p)
//---------------------------------------------------------------
int i2c_initMagSensor(pList *p)
{
    int rv = 0;
    int command = PMMODE_ALL;
    // Setup the Mag sensor register initial state here.
    if(p->samplingMode == POLL)                                         // (p->samplingMode == POLL [default])
    {
        rv = i2c_pololu_write_to(p->adapter, p->magAddr, RM3100_MAG_POLL, (uint8_t *) &command, 1);       //(XYZ_BUFLEN + 1)
        if(rv < 0)
        {
            showErrorMsg(rv);
        }
        return true;
    }
    return FALSE;
}

#if(USE_POLOLU)
#else
#endif

//---------------------------------------------------------------
// i2c_close()
//---------------------------------------------------------------
void i2c_close(pList *p)
{
    i2c_pololu_disconnect(p->adapter);
}
