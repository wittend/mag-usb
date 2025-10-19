//=========================================================================
// sensor_tests.c
//
// Routines to test sensor presence and communication.
//
// Author:      David Witten, KD0EAG
// Date:        October 10, 2025
// License:     GPL 3.0
//=========================================================================
#include "sensor_tests.h"
extern int RM3100_VER_EXPECTED;

//---------------------------------------------------------------
// scanforBusDevices(pList *p)
//---------------------------------------------------------------
int scanforBusDevices(pList *p)
{
    fprintf(stdout,"\nChecking Pololu I2C Adapter info:\n");

    // Scan for I2C devices
    fprintf(stdout,"\n  Scanning for I2C devices...\n");
    uint8_t found_addresses[128];
    int device_count = i2c_pololu_scan(p->adapter, found_addresses, 128);
    if (device_count > 0)
    {
        printf("    Found %d device(s):\n", device_count);
        for (int i = 0; i < device_count; ++i)
        {
            fprintf(stdout,"      Address: 0x%02X\n", found_addresses[i]);
        }
        return device_count;
    }
    else if (device_count == 0)
    {
        fprintf(stderr,"      No I2C devices found.\n");
        return -1;
    }
    else
    {
        fprintf(stderr, "    An error occurred during the I2C scan: %s\n", i2c_pololu_error_string(device_count));
        return -1;
    }
}

//---------------------------------------------------------------
// int verifyPololuAdaptor(pList *p)
//---------------------------------------------------------------
int verifyPololuAdaptor(pList *p)
{
    fprintf(stdout,"\nChecking Pololu I2C Adapter info:\n");
    //-----------------------------------------------------
    // Get the Pololu i2c device info.
    //-----------------------------------------------------
    i2c_pololu_device_info info;
    if(i2c_pololu_get_device_info(p->adapter, &info) == 0)
    {
        fprintf(stdout,"  Device Info:\n");
        fprintf(stdout,"     Vendor ID:        0x%04X\n", info.vendor_id);
        fprintf(stdout,"     Product ID:       0x%04X\n", info.product_id);
        fprintf(stdout,"     Firmware Version: %s\n", info.firmware_version);
        fprintf(stdout,"     Serial Number:    %s\n", info.serial_number);
    }
    else
    {
        fprintf(stdout, "    Failed to get device info.\n");
        return -1;
    }
    return true;
}

//---------------------------------------------------------------
// int verifyMagSensor(pList *p)
//---------------------------------------------------------------
int verifyTempSensor(pList *p)
{
    fprintf(stdout, "\nVerifying Temperature Sensor Status & Version...\n");

    if(i2c_pololu_is_connected(p->adapter))
    {
        fprintf(stdout, "  Connection to: %s is OK!\n", p->portpath);
    }
    else
    {
        return 0;
    }

    i2c_pololu_clear_bus(p->adapter);

    // uint8_t buf[2] = {0};
    // uint8_t addr = 0x23; // example device
    // uint8_t reg  = 0x36; // example register
    // int rv = -1;
    //
    // // Write register index
    // rv = i2c_pololu_write_to(p->adapter, addr, reg, "", 1);
    // if (rv < 0)
    // {
    //     fprintf(stdout, "  Write failed: %s\n", i2c_pololu_error_string(-rv));
    //     goto done;
    // }
    // // Read back 2 bytes
    // //    uint8_t buf[2] = {0};
    // rv = i2c_pololu_read_from(p->adapter, addr, reg, buf, 2);
    // if (rv < 0)
    // {
    //     fprintf(stdout, "  Read failed: %s\n", i2c_pololu_error_string(-rv));
    //     goto done;
    // }
    // //fprintf(stderr,"  Data: %02X %02X\n", buf[0], buf[1]);
    // rv = buf[0];
    // if(rv == (uint8_t) RM3100_VER_EXPECTED)
    // {
    //     fprintf(stdout, "  Version is OK!: 0x%2X\n", rv);
    //     return 0;
    // }
    // else
    // {
    //     fprintf(stdout, "  Version does NOT match!\n");
    //     return rv;
    // }
done:
    return true;
}

//---------------------------------------------------------------
// int verifyMagSensor(pList *p)
//---------------------------------------------------------------
int verifyMagSensor(pList *p)
{
    fprintf(stdout, "\nVerifying Magnetometer Status & Version...\n");

    if(i2c_pololu_is_connected(p->adapter))
    {
        fprintf(stdout, "  Connection to: %s is OK!\n", p->portpath);
    }
    else
    {
        return 0;
    }

    i2c_pololu_clear_bus(p->adapter);

    uint8_t buf[2] = {0};
    uint8_t addr = 0x23; // example device
    uint8_t reg  = 0x36; // example register
    int rv = -1;

    // Write register index
    rv = i2c_pololu_write_to(p->adapter, addr, reg, "", 1);
    if (rv < 0)
    {
        fprintf(stdout, "  Write failed: %s\n", i2c_pololu_error_string(-rv));
        goto done;
    }
    // Read back 2 bytes
    //    uint8_t buf[2] = {0};
    rv = i2c_pololu_read_from(p->adapter, addr, reg, buf, 2);
    if (rv < 0)
    {
        fprintf(stdout, "  Read failed: %s\n", i2c_pololu_error_string(-rv));
        goto done;
    }
    //fprintf(stderr,"  Data: %02X %02X\n", buf[0], buf[1]);
    rv = buf[0];
    if(rv == (uint8_t) RM3100_VER_EXPECTED)
    {
        fprintf(stdout, "  Version is OK!: 0x%2X\n", rv);
        return 0;
    }
    else
    {
        fprintf(stdout, "  Version does NOT match!\n");
        return rv;
    }
done:
    return true;
}

