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
#include "MCP9808.h"
extern int RM3100_VER_EXPECTED;

//---------------------------------------------------------------
// i2c_scanForBusDevices(pList *p)
//---------------------------------------------------------------
int i2c_scanForBusDevices(pList *p)
{
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
// int i2c_verifyPololuAdaptor(pList *p)
//---------------------------------------------------------------
int i2c_verifyPololuAdaptor(pList *p)
{
    fprintf(stdout,"\nChecking Pololu I2C Adapter info:\n");
    i2c_pololu_device_info info;
    if(i2c_pololu_get_device_info(p->adapter, &info) == 0)
    {
        if((info.vendor_id  == 0x1FFB) && ((info.product_id == 0x2502) || (info.product_id == 0x2503)))
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------
// int i2c_getAdaptorInfo(pList *p)
//---------------------------------------------------------------
int i2c_getAdaptorInfo(pList *p)
{
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
    return 0;
}

//---------------------------------------------------------------
// int i2c_verifyTempSensor(pList *p)
//---------------------------------------------------------------
int i2c_verifyTempSensor(pList *p)
{
    fprintf(stdout, "\nVerifying Temperature Sensor Status & Version...\n");

    if(!i2c_pololu_is_connected(p->adapter))
    {
        fprintf(stderr, "  Adapter not connected.\n");
        return 1; // non-zero on failure per requirement
    }
    // Clear the bus before transactions
    (void)i2c_pololu_clear_bus(p->adapter);

    int rc;
    uint8_t buf[2] = {0};

    // Read Manufacturer ID (0x06), 2 bytes MSB first
    rc = i2c_pololu_read_from(p->adapter, (uint8_t)p->remoteTempAddr, MCP9808_REG_MANUF_ID, buf, 2);
    if(rc < 0)
    {
        fprintf(stderr, "  Failed to read MCP9808_MANUF_ID: %s\n", i2c_pololu_error_string(-rc));
        return 1;
    }
    uint16_t manuf = (uint16_t)((buf[0] << 8) | buf[1]);
    fprintf(stdout, "  MCP9808 MANUF_ID: 0x%04X (expected 0x%04X)\n", manuf, MCP9808_MANID_EXPECTED);

    // Read Device ID/Revision (0x07), 2 bytes MSB first
    rc = i2c_pololu_read_from(p->adapter, (uint8_t)p->remoteTempAddr, MCP9808_REG_DEVICE_ID, buf, 2);
    if(rc < 0)
    {
        fprintf(stderr, "  Failed to read MCP9808_DEVICE_ID: %s\n", i2c_pololu_error_string(-rc));
        return 1;
    }
    uint16_t devid = (uint16_t)((buf[0] << 8) | buf[1]);
    // Mask off revision bits (lower 8 bits) and accept either MCP9808 or MCP9804 device IDs
    const uint16_t REV_MASK = 0x00FF; // lower byte is revision per datasheets
    uint16_t devid_masked = (uint16_t)(devid & ~REV_MASK);
    uint16_t expected_9808_masked = (uint16_t)(MCP9808_DEVREV_EXPECTED & ~REV_MASK);
    uint16_t expected_9804_masked = (uint16_t)(MCP9804_DEVREV_EXPECTED & ~REV_MASK);
    fprintf(stdout, "  Temp SENSOR DEVICE_ID: 0x%04X (accept 0x%04X or 0x%04X, ignoring rev 0x%02X)\n",
            devid,
            expected_9808_masked,
            expected_9804_masked,
            (unsigned)(devid & REV_MASK));

    int err = 0;
    if(manuf != MCP9808_MANID_EXPECTED)
    {
        err |= 0x01; // manufacturer mismatch
    }
    if(!(devid_masked == expected_9808_masked || devid_masked == expected_9804_masked))
    {
        err |= 0x02; // device id mismatch
    }

    if(err == 0)
    {
        fprintf(stdout, "  Temperature sensor identification OK (MCP9808/MCP9804).\n");
        return 0; // success
    }

    // Non-zero error code; provide detail
    if(err & 0x01) fprintf(stderr, "  ERROR: Manufacturer ID mismatch.\n");
    if(err & 0x02) fprintf(stderr, "  ERROR: Device ID mismatch (not 9808/9804) or invalid.\n");
    return err;
}

//---------------------------------------------------------------
// int i2c_verifyMagSensor(pList *p)
//---------------------------------------------------------------
int i2c_verifyMagSensor(pList *p)
{
    fprintf(stdout, "\nVerifying Magnetometer Status & Version...\n");

    i2c_pololu_clear_bus(p->adapter);

    uint8_t buf[2] = {0};
    uint8_t addr = 0x23; // example device
    uint8_t reg  = 0x36; // example register
    int rv = -1;

    // Write register index (no payload bytes)
    rv = i2c_pololu_write_to(p->adapter, addr, reg, NULL, 0);
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

