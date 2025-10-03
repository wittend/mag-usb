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
//=========================================================================
// backends/i2c_pololu.c
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"
#include "i2c.h"
#include "i2c-pololu.h" // your Pololu API headers

int i2c_open(pList *p, const char *portpath)
{
    // Map to Pololu open/init sequence
    struct stat sb;
    if(!stat(p->portpath, &sb))
    {
       i2c_pololu_adapter *pAdapt = p->adapter;
       return i2c_pololu_connect(pAdapt, p->portpath);
    }
    else
    {
        char errstr[1024]= "";
        sprintf(errstr, "Failed to open port %s. Exiting.", p->portpath);
        perror(errstr);
        exit(EXIT_FAILURE);
    }
}

//void i2c_init(pList *p, int adaptor)
void i2c_init(pList *p)
{
//    pololu_i2c_adapter adapter;
    i2c_pololu_init(p->adapter);
}

void i2c_setAddress(pList *p, int devAddr)
{
//    p->adapter.fd; (void)devAddr;
}

void i2c_setBitRate(pList *p, int devspeed)
{
//    p->adapter.fd; (void)devspeed;
}

#if(USE_POLOLU)

//-----------------------------------------------------------------------------
// Calls for the Temperature sensor.
//-----------------------------------------------------------------------------

int i2c_write_temp(pList *p, uint8_t reg, uint8_t value)
{
    int rv = 0;
    rv = i2c_pololu_write_to(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, &value, (uint8_t) 1);
    return value;
}

uint8_t i2c_read_temp(pList *p, uint8_t reg)
{
    uint8_t  rv;
    i2c_pololu_read_from(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) &rv, (uint8_t) 1 );
    return rv;
}

int i2c_writebyte_temp(pList *p, uint8_t reg, char* buffer, short int length)
{
    return i2c_pololu_write_to(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) 1);
}

int i2c_reabyte_temp(pList *p, uint8_t reg, uint8_t* buf, short int length)
{
    return i2c_pololu_read_from(p->adapter, (uint8_t) p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) 1);
}

int i2c_writebuf_temp(pList *p, uint8_t reg, char* buf, short int length)
{
    return i2c_pololu_write_to(p->adapter,(uint8_t) p->remoteTempAddr, (uint8_t) reg, buf, (uint8_t) length);
}

int i2c_readbuf_temp(pList *p, uint8_t reg, uint8_t *buf, uint8_t length)
{
    return i2c_pololu_read_from(p->adapter, p->remoteTempAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) length );
}

//-----------------------------------------------------------------------------
// Calls for the Magnetometer
//-----------------------------------------------------------------------------

int i2c_write_mag(pList *p, uint8_t reg, uint8_t value)
{
    int rv = 0;
    rv = i2c_pololu_write_to(p->adapter, (uint8_t)p->magAddr, (uint8_t) reg, &value, (uint8_t) 1);
    return value;
}

uint8_t i2c_read_mag(pList *p, uint8_t reg)
{
    uint8_t  rv;
    i2c_pololu_read_from(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) &rv, (uint8_t) 1 );
    return rv;
}

int i2c_writebyte_mag(pList *p, uint8_t reg, char* buffer, short int length)
{
    return i2c_pololu_write_to(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) 1);
}

int i2c_reabyte_mag(pList *p, uint8_t reg, uint8_t* buf, short int length)
{
    return i2c_pololu_read_from(p->adapter, (uint8_t) p->magAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) 1);
}

int i2c_writebuf_mag(pList *p, uint8_t reg, char* buf, short int length)
{
    return i2c_pololu_write_to(p->adapter,(uint8_t) p->magAddr, (uint8_t) reg, buf, (uint8_t) length);
}

int i2c_readbuf_mag(pList *p, uint8_t reg, uint8_t *buf, uint8_t length)
{
    return i2c_pololu_read_from(p->adapter, p->magAddr, (uint8_t) reg, (uint8_t *) buf, (uint8_t) length );
}

#else
#endif

void i2c_close(pList *p)
{
    i2c_pololu_disconnect(p->adapter);
}
