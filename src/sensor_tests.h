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

int  i2c_verifyPololuAdaptor(pList *p);
int  i2c_getAdaptorInfo(pList *p);
int  i2c_scanForBusDevices(pList *p);
int  i2c_verifyMagSensor(pList *p);
int  i2c_verifyTempSensor(pList *p);

#endif // MAG_USB_SENSOR_TESTS_H
