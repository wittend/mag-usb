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
//#include "main.h"
#include "i2c.h"
#include "i2c_pololu.h" // your Pololu API headers

// int pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t size );
// int pololu_i2c_read_from( pololu_i2c_adapter *adapter, uint8_t address, uint8_t *data, uint8_t size );

int i2c_open(pList *p, const char *portname)
{
    // Map to Pololu open/init sequence
    // int pololu_i2c_connect( pololu_i2c_adapter *adapter, const char *port_name )
    //    return pololu_i2c_connect((pololu_i2c_adapter *)p->adapter, portname);
    //    return pololu_i2c_connect(&(p->adapter), portname);
    pololu_i2c_adapter pAdapt = p->adapter;
    return pololu_i2c_connect(pAdapt, portname);
}

void i2c_init(pList *p, int adaptor)
{
//    pololu_i2c_adapter adapter;
    pololu_i2c_init(&(p->adapter));
}

void i2c_setAddress(pList *p, int devAddr)
{
    p->adapter.fd; (void)devAddr;
}

void i2c_setBitRate(pList *p, int devspeed)
{
    p->adapter.fd; (void)devspeed;
}

int i2c_write(pList *p, uint8_t reg, uint8_t value)
{
    // Translate to Pololu call
    (void)p; (void)reg; (void)value;
//    int pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t size );
    return pololu_i2c_write_to( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) value, (uint8_t) 1);
//    return 0;
}

uint8_t i2c_read(pList *p, uint8_t reg)
{
    (void)p->adapter.fd; (void)reg;
    uint8_t  rv;
//int pololu_i2c_read_from( pololu_i2c_adapter *adapter, uint8_t address, uint8_t *data, uint8_t size );
    return pololu_i2c_read_from( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) &rv, (uint8_t) 1 );
    return rv;
}

int i2c_writebyte(pList *p, uint8_t reg, char* buffer, short int length)
{
    (void)p->adapter.fd; (void)reg; (void)buffer; (void)length;
    return pololu_i2c_write_to( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) 1 );
    return 0;
}

int i2c_reabyte(pList *p, uint8_t reg, uint8_t* buf, short int length)
{
    (void)p->adapter.fd; (void)reg; (void)buf; (void)length;
//t pololu_i2c_read_from( pololu_i2c_adapter *adapter, uint8_t address, uint8_t *data, uint8_t size );
    return pololu_i2c_read_from( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) buf, (uint8_t) 1 );
//    return 0;
}

int i2c_writebuf(pList *p, uint8_t reg, char* buffer, short int length)
{
    (void)p->adapter.fd; (void)reg; (void)buffer; (void)length;
    return pololu_i2c_write_to( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) buffer, (uint8_t) length );
//    return 0;
}

int i2c_readbuf(pList *p, uint8_t reg, uint8_t* buf, char *length)
{
//    (void)p->adapter.fd; (void)reg; (void)buf; (void)length;
    return pololu_i2c_read_from( (pololu_i2c_adapter *) p->adapter.fd, (uint8_t) reg, (uint8_t *) buf, (uint8_t) length );
//    return 0;
}

void i2c_close(pList *p)
{
    (void)p->adapter.fd;
}
