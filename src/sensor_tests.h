//=========================================================================
// sensor_tests.c
//
// Routines to test sensor presence and communication.
//
// Author:      David Witten, KD0EAG
// Date:        October 10, 2025
// License:     GPL 3.0
//=========================================================================
#ifndef MAG_USB_SENSOR_TESTS_H
#define MAG_USB_SENSOR_TESTS_H

#include "main.h"
//#include "rm3100.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

int  verifyPololuAdaptor(pList *p);
int  scanforBusDevices(pList *p);
int  verifyMagSensor(pList *p);
int  verifyTempSensor(pList *p);

#endif // MAG_USB_SENSOR_TESTS_H
