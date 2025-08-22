//=========================================================================
// cmdmgr.h
//
// header file for commandline / configuration management for runMag utility.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// License:     GPL 3.0
// Note:        replaces i2c.c (using file system calls to read(), write(), etc.
//              with calls to pigpio. 
//              Also adding callbacks on GPIO27 for PPS rising edge.
//=========================================================================
#ifndef SWX3100CMDMGR_h
#define SWX3100CMDMGR_h

//#include "magdata.h"
#include "main.h"

//------------------------------------------
// Prototypes
//------------------------------------------
long currentTimeMillis();
struct tm *getUTC();
// void showCountGainRelationship();
void showSettings(ctlList *p);
int getCommandLine(int argc, char** argv, ctlList *p);


#endif // SWX3100CMDMGR_h
