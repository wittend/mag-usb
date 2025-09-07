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

#include "i2c_pololu.h" // your Pololu API headers
#include "main.h"

//------------------------------------------
// Prototypes
//------------------------------------------
int i2c_open(pList *p, const char *portname);
void i2c_init(pList *p);
void i2c_setAddress(pList *p, int devAddr);
void i2c_setBitRate(pList *p, int devspeed);
int i2c_write(pList *p, uint8_t reg, uint8_t value);
uint8_t i2c_read(pList *p, uint8_t reg);
int i2c_writebyte(pList *p, uint8_t reg, char* buffer, short int length);
int i2c_reabyte(pList *p, uint8_t reg, uint8_t* buf, short int length);
int i2c_writebuf(pList *p, uint8_t reg, char* buffer, short int length);
//int i2c_readbuf(pList *p, uint8_t reg, uint8_t buf, char *length);
int i2c_readbuf(pList *p, uint8_t reg, uint8_t *buf, uint8_t length);
void i2c_close(pList *p);
#endif //PNIRM3100_I2C_H
