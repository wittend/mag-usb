//=========================================================================
// i2c.h
//
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
// 
// Author:      David Witten, KD0EAG
// Date:        April 21, 2020
// License:     GPL 3.0
//=========================================================================
#ifndef PNIRM3100_I2C_H
#define PNIRM3100_I2C_H

#include "../main.h"
#include "i2c_pololu.h" // your Pololu API headers

//------------------------------------------
// Prototypes
//------------------------------------------
int i2c_open(ctlList *p, const char *porthame);
void i2c_init(ctlList *p, int adaptor);
void i2c_setAddress(ctlList *p, int devAddr);
void i2c_setBitRate(ctlList *p, int devspeed);
int i2c_write(ctlList *p, uint8_t reg, uint8_t value);
uint8_t i2c_read(ctlList *p, uint8_t reg);
int i2c_writebyte(ctlList *p, uint8_t reg, char* buffer, short int length);
int i2c_reabyte(ctlList *p, uint8_t reg, uint8_t* buf, short int length);
int i2c_writebuf(ctlList *p, uint8_t reg, char* buffer, short int length);
int i2c_readbuf(ctlList *p, uint8_t reg, uint8_t* buf, char *length);
void i2c_close(ctlList *p);
#endif //PNIRM3100_I2C_H
