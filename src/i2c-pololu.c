// backends/i2c_pigpio.c
#include <stdint.h>
//#include "i2c.h"

int i2c_open(const char* portname)
{
    // Translate portname (or ignore if pigpio selects bus by number elsewhere)
    // Example pigpio: int handle = i2cOpen(bus, addr, flags);
    // You may store/track fd->(bus, addr) mapping if your API expects separate calls.
    // For demonstration, return a fake fd/handle or actual handle from pigpio.
    return pololu_i2c_open();
}

void i2c_init(int adaptor)
{
    // pigpio requires gpioInitialise(); store result if needed
    // gpioInitialise();
    (void)adaptor;
}

void i2c_setAddress(int fd, int devAddr)
{
    // If your unified API separates open and setAddress but pigpio wants both at once,
    // you can close and reopen, or maintain state and rebind. Or document that open
    // is a no-op and setAddress performs i2cOpen.
    (void)fd;
    (void)devAddr;
}

void i2c_setBitRate(int fd, int devspeed)
{
    // pigpio supports smbus/i2c flags; map devspeed if supported, else ignore.
    (void)fd;
    (void)devspeed;
}

int i2c_write(int fd, uint8_t reg, uint8_t value)
{
    // return i2cWriteByteData(fd, reg, value);
    (void)fd;
    (void)reg;
    (void)value;
    return 0;
}

uint8_t i2c_read(int fd, uint8_t reg)
{
    // int rv = i2cReadByteData(fd, reg);
    // return (rv >= 0) ? (uint8_t)rv : 0;
    (void)fd;
    (void)reg;
    return 0;
}

int i2c_writebyte(int fd, uint8_t reg, char* buffer, short int length)
{
    // return i2cWriteI2CBlockData(fd, reg, (char*)buffer, length);
    (void)fd;
    (void)reg;
    (void)buffer;
    (void)length;
    return 0;
}

int i2c_reabyte(int fd, uint8_t reg, uint8_t* buf, short int length)
{
    // return i2cReadI2CBlockData(fd, reg, (char*)buf, length);
    (void)fd;
    (void)reg;
    (void)buf;
    (void)length;
    return 0;
}

int i2c_writebuf(int fd, uint8_t reg, char* buffer, short int length)
{
    // same as writebyte if your API intends a block write
    (void)fd;
    (void)reg;
    (void)buffer;
    (void)length;
    return 0;
}

int i2c_readbuf(int fd, uint8_t reg, uint8_t* buf, char* length)
{
    // read into buf, set *length to actually read count if needed
    (void)fd;
    (void)reg;
    (void)buf;
    (void)length;
    return 0;
}

void i2c_close(int fd)
{
    // i2cClose(fd);
    (void)fd;
}
