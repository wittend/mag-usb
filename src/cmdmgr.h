//=========================================================================
// cmdmgr.h
//
// header file for commandline / configuration management for runMag utility.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// License:     GPL 3.0
//=========================================================================
#ifndef SWX3100CMDMGR_h
#define SWX3100CMDMGR_h


//------------------------------------------
// Prototypes
//------------------------------------------
void showSettings(pList *p);
int getCommandLine(int argc, char** argv, pList *p);


#endif // SWX3100CMDMGR_h
