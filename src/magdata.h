//=========================================================================
// main.h
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
#ifndef SWX3100MAGData_h
#define SWX3100MAGData_h

#include "../main.h"

//------------------------------------------
// Prototypes
//------------------------------------------
//int openI2CBus(pList *p);
//void closeI2CBus(int i2c_fd);
//int  setNOSReg(pList *p);
//unsigned short setMagSampleRate(pList *p, unsigned short sample_rate);
unsigned short getMagSampleRate(pList *p);
unsigned short getCCGainEquiv(unsigned short CCVal);
int  startCMM(pList *p);
// int  getMagRev(pList *p);
// int setup_mag(pList *p);
int  runBIST(pList *p);
int  getCMMReg(pList *p);
void setCMMReg(pList *p);
int  getTMRCReg(pList *p);
void setTMRCReg(pList *p);
void setCycleCountRegs(pList *p);
void readCycleCountRegs(pList *p);

//int readTemp(pList *p);
//int readMagCMM(pList *p, int32_t *XYZ);
//int readMagPOLL(pList *p, int32_t *XYZ);

#endif // SWX3100MAGData_h
