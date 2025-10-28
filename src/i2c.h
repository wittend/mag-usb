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

#include "main.h"

//------------------------------------------
// Prototypes
//------------------------------------------
//int i2c_open(pList *p, const char *portname);
int i2c_open(pList *p);
int i2c_init(pList *p);
void i2c_setAddress(pList *p, int devAddr);
void i2c_setBitRate(pList *p, int devspeed);

int i2c_initMagSensor(pList *p);

int  i2c_readRemoteTemp(pList *p);
//int  readMagCMM(volatile pList *p);
int  i2c_readMagPOLL(pList *p);

int i2c_write_mag(pList *p,     uint8_t reg, uint8_t value);
uint8_t i2c_read_mag(pList *p,  uint8_t reg);
int i2c_writebyte_mag(pList *p, uint8_t reg, char* buffer, short int length);
int i2c_reabyte_mag(pList *p,   uint8_t reg, uint8_t* buf, short int length);
int i2c_writebuf_mag(pList *p,  uint8_t reg, char* buffer, short int length);
int i2c_readbuf_mag(pList *p,   uint8_t reg, uint8_t *buf, uint8_t length);

// int initTempSensors(pList *p)
int i2c_write_temp(pList *p,     uint8_t reg, uint8_t value);
uint8_t i2c_read_temp(pList *p,  uint8_t reg);
int i2c_writebyte_temp(pList *p, uint8_t reg, char* buffer, short int length);
int i2c_reabyte_temp(pList *p,   uint8_t reg, uint8_t* buf, short int length);
int i2c_writebuf_temp(pList *p,  uint8_t reg, char* buffer, short int length);
int i2c_readbuf_temp(pList *p,   uint8_t reg, uint8_t *buf, uint8_t length);

void i2c_close(pList *p);


#endif //PNIRM3100_I2C_H
