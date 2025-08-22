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
//int openI2CBus(ctlList *p);
//void closeI2CBus(int i2c_fd);
//int  setNOSReg(ctlList *p);
//unsigned short setMagSampleRate(ctlList *p, unsigned short sample_rate);
unsigned short getMagSampleRate(ctlList *p);
unsigned short getCCGainEquiv(unsigned short CCVal);
int  startCMM(ctlList *p);
// int  getMagRev(ctlList *p);
// int setup_mag(ctlList *p);
int  runBIST(ctlList *p);
int  getCMMReg(ctlList *p);
void setCMMReg(ctlList *p);
int  getTMRCReg(ctlList *p);
void setTMRCReg(ctlList *p);
void setCycleCountRegs(ctlList *p);
void readCycleCountRegs(ctlList *p);

//int readTemp(ctlList *p);
//int readMagCMM(ctlList *p, int32_t *XYZ);
//int readMagPOLL(ctlList *p, int32_t *XYZ);

#endif // SWX3100MAGData_h
