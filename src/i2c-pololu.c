//=========================================================================
// i2c_pololu.c
//
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
//
// Author:      David Witten, KD0EAG
// Date:        7/17/25
// License:     GPL 3.0
//=========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include "i2c-pololu.h"

//------------------------------------------
// i2c_pololu_check_device_available()
//------------------------------------------
int i2c_pololu_check_device_available(const char* path, int timeout_ms)
{
    if(path == NULL || *path == '\0')
    {
        return -EINVAL;
    }

    struct timespec start_ts = {0}, now = {0};
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    const int sleep_ms = 50;
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = sleep_ms * 1000000L;

    for(;;)
    {
        struct stat sb;
        if(stat(path, &sb) == 0)
        {
            if(!S_ISCHR(sb.st_mode))
            {
                return -ENOTTY; // not a character device
            }
            // Try a non-blocking open to check availability
            int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_EXCL);
            if(fd >= 0)
            {
                close(fd);
                return 0; // available
            }
            else
            {
                if(errno == EBUSY || errno == EACCES || errno == EPERM)
                {
                    // May be in-use or permission not yet ready (udev rules), allow retries until timeout
                }
                else
                {
                    return -errno; // other errors: pass through
                }
            }
        }
        else
        {
            if(errno != ENOENT)
            {
                return -errno; // unexpected stat error
            }
            // if ENOENT, allow retry until timeout
        }

        // Check timeout
        if(timeout_ms <= 0)
        {
            return -ENOENT; // single-shot or zero timeout
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (long)((now.tv_sec - start_ts.tv_sec) * 1000L + (now.tv_nsec - start_ts.tv_nsec) / 1000000L);
        if(elapsed_ms >= timeout_ms)
        {
            // If last stat said exists but open failed with EBUSY, report busy; otherwise ENOENT
            // Try a final stat to decide
            if(stat(path, &sb) == 0)
            {
                return -EBUSY; // treat as in-use
            }
            return -ENOENT;
        }
        // Sleep a bit before retry
        nanosleep(&req, NULL);
    }
}

//------------------------------------------
// check_response()
//------------------------------------------
static int check_response( const uint8_t *response, size_t expected_len, size_t actual_len )
{
    if(actual_len < expected_len)
    {
        fprintf(OUTPUT_ERROR, "Timeout while reading response from adapter (received %zu bytes).\n", actual_len);
        return -ERROR_TIMEOUT;
    }
    if(response[0] != ERROR_NONE)
    {
        return -response[0];
    }
    return 0; // Success
}

//------------------------------------------
// i2c_pololu_init()
// This just initialixes the dile descriptor.  not much use, really.
//------------------------------------------
int i2c_pololu_init( i2c_pololu_adapter *adapter )
{
    if(adapter)
    {
        adapter->fd = -1;
        return 0;
    }
    return 1;
}

//------------------------------------------
// i2c_pololu_connect()
//------------------------------------------
int i2c_pololu_connect( i2c_pololu_adapter *adapter, const char *port_name )
{
    if(!adapter || !port_name)
    {
        return -1;
    }
    // Pre-flight availability check (wait up to 500 ms)
    int avail = i2c_pololu_check_device_available(port_name, 500);
    if(avail != 0)
    {
        char eBuf[1024] = "";
        #if defined(__GLIBC__) && defined(_GNU_SOURCE)
            char *errmsg = strerror_r(-avail, eBuf, sizeof eBuf);
            snprintf(eBuf, sizeof eBuf, "%s", errmsg);
        #else
            int _ignored = strerror_r(-avail, eBuf, sizeof eBuf);
            (void)_ignored;
        #endif
        fprintf(OUTPUT_ERROR, "Error opening port device may not exist or may be in use: %s. %s\n", port_name, eBuf);
        return -1;
    }
    adapter->fd = open(port_name, O_RDWR | O_NOCTTY | O_EXCL);
    if(adapter->fd < 0)
    {
        char eBuf[1024] = "";
        #if defined(__GLIBC__) && defined(_GNU_SOURCE)
            char *errmsg = strerror_r(errno, eBuf, sizeof eBuf);
            snprintf(eBuf, sizeof eBuf, "%s", errmsg);
        #else
            int _ignored2 = strerror_r(errno, eBuf, sizeof eBuf);
            (void)_ignored2;
        #endif
        fprintf(OUTPUT_ERROR, "Error opening port device %s. %s\n", port_name, eBuf);
        return -1;
    }
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if(tcgetattr(adapter->fd, &tty) != 0)
    {
        perror("Error from tcgetattr");
        close(adapter->fd);
        adapter->fd = -1;
        return -1;
    }
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    // cfsetospeed(&tty, B38400);
    // cfsetispeed(&tty, B38400);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK; // disable break processing
    tty.c_lflag = 0; // no signaling chars, no echo
    tty.c_oflag = 0; // no remapping, no delays
    tty.c_cc[VMIN] = 0; // read doesn't block
    tty.c_cc[VTIME] = 1; // 0.1 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD); // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if(tcsetattr(adapter->fd, TCSANOW, &tty) != 0)
    {
        perror("Error from tcsetattr");
        close(adapter->fd);
        adapter->fd = -1;
        return -1;
    }
    // Discard old received data
    tcflush(adapter->fd, TCIFLUSH);
    return 0;
}

//------------------------------------------
// i2c_pololu_disconnect()
//------------------------------------------
void i2c_pololu_disconnect( i2c_pololu_adapter *adapter )
{
    if(adapter && adapter->fd >= 0)
    {
        close(adapter->fd);
        adapter->fd = -1;
    }
}

//------------------------------------------
// i2c_pololu_is_connected()
//------------------------------------------
bool i2c_pololu_is_connected( const i2c_pololu_adapter *adapter )
{
    return adapter && adapter->fd >= 0;
}

//------------------------------------------
// i2c_pololu_write_to()
//------------------------------------------
int i2c_pololu_write_to( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, const uint8_t *data, uint8_t size )
{
    if(!i2c_pololu_is_connected(adapter))
    {
        return -1;
    }
    // size is uint8_t, so it cannot exceed 255; no range check needed here.

    uint8_t cmd[258];
    cmd[0] = CMD_I2C_WRITE;
    cmd[1] = address;
    cmd[2] = 1 + size;  // Length includes register byte + data
    cmd[3] = reg;
    memcpy(&cmd[4], data, size);
    if(write(adapter->fd, cmd, size + 4) != size + 4)
    {
        perror("Failed to write to adapter");
        return -1;
    }
    uint8_t response[1];
    ssize_t len = read(adapter->fd, response, 1);
    int error = check_response(response, 1, len);
    if(error)
    {
        return error;
    }

    return size;
}

//------------------------------------------
// i2c_pololu_read_from()
//------------------------------------------
int i2c_pololu_read_from( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, uint8_t *data, uint8_t size )
{
    if(!i2c_pololu_is_connected(adapter))
    {
        return -1;
    }
    
    // First, write the register address to the device
    uint8_t write_cmd[4];
    write_cmd[0] = CMD_I2C_WRITE;
    write_cmd[1] = address;
    write_cmd[2] = 1;  // Writing 1 byte (the register address)
    write_cmd[3] = reg;
    
    if(write(adapter->fd, write_cmd, 4) != 4)
    {
        perror("Failed to write register address to adapter");
        return -1;
    }
    
    // Read the write response
    uint8_t write_response[1];
    ssize_t len = read(adapter->fd, write_response, 1);
    int error = check_response(write_response, 1, len);
    if(error)
    {
        fprintf(OUTPUT_ERROR, "Error writing register address: %s\n", i2c_pololu_error_string(error));
        return error;
    }
    
    // Now read from the device
    uint8_t read_cmd[3];
    read_cmd[0] = CMD_I2C_READ;
    read_cmd[1] = address;
    read_cmd[2] = size;

    if(write(adapter->fd, read_cmd, 3) != 3)
    {
        perror("Failed to write read command to adapter");
        return -1;
    }

    uint8_t response[256];
    len = read(adapter->fd, response, 1 + size);  // Error byte + data
    error = check_response(response, 1 + size, len);
    if(error)
    {
        fprintf(OUTPUT_ERROR, "Error reading from device: %s\n", i2c_pololu_error_string(error));
        return error;
    }
    memcpy(data, &response[1], size);  // Skip error byte, copy data
    return size;
}

//------------------------------------------
// i2c_pololu_write_and_read_from()
//------------------------------------------
int i2c_pololu_write_and_read_from( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, uint8_t *data, uint8_t size )
{
    if(!i2c_pololu_is_connected(adapter))
    {
        return -1;
    }

    uint8_t cmd[258];
    cmd[0] = CMD_I2C_WRITE_AND_READ;
    cmd[1] = address;
    cmd[2] = 1;     // Write 1 byte (register address)
    cmd[3] = size;  // Read 'size' bytes
    cmd[4] = reg;   // Register to read from

    if(write(adapter->fd, cmd, 5) != 5)
    {
        perror("Failed to write to adapter");
        return -1;
    }
    
    uint8_t response[256];
    ssize_t len = read(adapter->fd, response, 1 + size);  // Error byte + data
    int error = check_response(response, 1 + size, len);
    if(error)
    {
        fprintf(OUTPUT_ERROR, "Error in write-and-read: %s\n", i2c_pololu_error_string(error));
        return error;
    }
    
    memcpy(data, &response[1], size);  // Skip error byte, copy data
    return size;
}

/*
Minor functions currently not implemented.

//------------------------------------------
// i2c_pololu_set_i2c_mode()
//------------------------------------------
int i2c_pololu_set_i2c_mode(int mode)
{
   uint8_t cmd[] = { CMD_I2C_READ, address, size };
   return 0;
}

//------------------------------------------
// i2c_pololu_set_timeout()
//------------------------------------------
int i2c_pololu_set_timeout()
{
    uint8_t cmd[] = { CMD_I2C_READ, address, size };
    return 0;
}

//------------------------------------------
// i2c_pololu_set_STM32_timing()
//------------------------------------------
int i2c_pololu_set_STM32_timing()
{
    uint8_t cmd[] = { CMD_I2C_READ, address, size };
    return 0;
}

//------------------------------------------
// i2c_pololu_digital_read()
//------------------------------------------
int i2c_pololu_digital_read()
{
    uint8_t cmd[] = { CMD_I2C_READ, address, size };
    return 0;
}

//------------------------------------------
// i2c_pololu_enable_VCC_out()
//------------------------------------------
int i2c_pololu_enable_VCC_out()
{
    uint8_t cmd[] = { CMD_I2C_READ, address, size };
    return 0;
}

*/

//------------------------------------------
// i2c_pololu_set_frequency()
//------------------------------------------
int i2c_pololu_set_frequency( i2c_pololu_adapter *adapter, unsigned int frequency_khz )
{
    if(!i2c_pololu_is_connected(adapter))
    {
        return -1;
    }

    uint8_t mode;
    if(frequency_khz >= 1000)
    {
        mode = I2C_FAST_MODE_PLUS;
    }
    else if(frequency_khz >= 400)
    {
        mode = I2C_FAST_MODE;
    }
    else if(frequency_khz >= 100)
    {
        mode = I2C_STANDARD_MODE;
    }
    else
    {
        mode = I2C_10_KHZ;
    }

    uint8_t cmd[] = { CMD_SET_I2C_MODE, mode };
    if(write(adapter->fd, cmd, 2) != 2)
    {
        perror("Failed to set frequency");
        return -1;
    }
    return 0;
}

//------------------------------------------
// i2c_pololu_clear_bus()
//------------------------------------------
int i2c_pololu_clear_bus( i2c_pololu_adapter *adapter )
{
    if(!i2c_pololu_is_connected(adapter))
    {
        return -1;
    }
    uint8_t cmd = CMD_CLEAR_BUS;
    if(write(adapter->fd, &cmd, 1) != 1)
    {
        perror("Failed to clear bus");
        return -1;
    }
    return 0;
}

//------------------------------------------
// i2c_pololu_get_device_info()
//------------------------------------------
int i2c_pololu_get_device_info( i2c_pololu_adapter *adapter, i2c_pololu_device_info *info )
{
    if(!i2c_pololu_is_connected(adapter) || !info)
    {
        return -1;
    }
    uint8_t cmd = CMD_GET_DEVICE_INFO;
    if(write(adapter->fd, &cmd, 1) != 1)
    {
        fprintf(OUTPUT_ERROR, "Failed to request device info\n");
//        perror("Failed to request device info\n");
        return -1;
    }
    uint8_t length;
    if(read(adapter->fd, &length, 1) != 1)
    {
        fprintf(OUTPUT_ERROR, "Failed to read Pololu device info length.\n");
//        perror("Failed to read Pololu device info length.\n");
        return -1;
    }
    // Expect 28 bytes based on protocol (including length byte)
    const uint8_t expected_len = 28;
    if(length == 0 || length > expected_len)
    {
        fprintf(OUTPUT_ERROR, "Invalid device info length: %u (expected <= %u)\n", length, expected_len);
        return -1;
    }
    uint8_t raw_info[28];
    raw_info[0] = length;
    if(read(adapter->fd, &raw_info[1], length - 1) != length - 1)
    {
        fprintf(OUTPUT_ERROR, "Failed to read device info payload\n");
//        perror("Failed to read device info payload\n");
        return -1;
    }
    // Unpack the data
#pragma pack(push, 1)
    struct device_info_raw
    {
        uint8_t length;
        uint8_t version;
        uint16_t vendor_id;
        uint16_t product_id;
        uint16_t firmware_version_bcd;
        char firmware_modification[8];
        char serial_number[12];
    } *raw = (struct device_info_raw *) raw_info;
#pragma pack(pop)
    if(raw->version != 0)
    {
        fprintf(OUTPUT_ERROR, "Unrecognized device info version: %d\n", raw->version);
        return -1;
    }
    info->vendor_id = raw->vendor_id;
    info->product_id = raw->product_id;
    info->firmware_version_bcd = raw->firmware_version_bcd;
    snprintf(info->firmware_version, sizeof(info->firmware_version), "%x.%02x", (raw->firmware_version_bcd >> 8), (raw->firmware_version_bcd & 0xFF));
    memcpy(info->firmware_modification, raw->firmware_modification, 8);
    info->firmware_modification[8] = '\0';
    if(strcmp(info->firmware_modification, "-") == 0)
    {
        info->firmware_modification[0] = '\0';
    }
    // Format the serial number
    // Safely format: 6 groups of two bytes as hex, separated by dashes, no trailing dash
    size_t pos = 0;
    for(int i = 0; i < 6; ++i)
    {
        int written = snprintf(info->serial_number + pos, sizeof(info->serial_number) - pos,
                               (i < 5) ? "%02X%02X-" : "%02X%02X",
                               (uint8_t) raw->serial_number[i * 2], (uint8_t) raw->serial_number[i * 2 + 1]);
        if(written < 0)
        {
            return -1; // encoding error
        }
        pos += (size_t)written;
        if(pos >= sizeof(info->serial_number))
        {
            info->serial_number[sizeof(info->serial_number) - 1] = '\0';
            break;
        }
    }
    return 0;
}

//------------------------------------------
// i2c_pololu_scan()
//------------------------------------------
int i2c_pololu_scan( i2c_pololu_adapter *adapter, uint8_t *found_addresses, int max_devices )
{
    if(!i2c_pololu_is_connected(adapter) || !found_addresses || max_devices <= 0)
    {
        return -1;
    }

    uint8_t cmd_bytes[128 * 3];
    for(int i = 0; i < 128; ++i)
    {
        cmd_bytes[i * 3] = 0x91; // write_to
        cmd_bytes[i * 3 + 1] = i; // address
        cmd_bytes[i * 3 + 2] = 0; // 0-length
    }

    if(write(adapter->fd, cmd_bytes, sizeof(cmd_bytes)) != sizeof(cmd_bytes))
    {
        perror("Failed to send scan command");
        return -1;
    }

    uint8_t rPtr = 0;
    uint8_t responses[128];
    uint8_t bytes_left = sizeof(responses);
    ssize_t bytes_read = read(adapter->fd, &responses[rPtr], bytes_left);
    if(bytes_read == -1)
    {
        perror("Failed to read scan responses");
        return -1;
    }
    while(bytes_left > 0)
    {
        if(bytes_read == 0)
        {
            perror("Read returns EOF (0)");
            break;
        }
        rPtr += bytes_read;
        bytes_left -= bytes_read;
        bytes_read = read(adapter->fd, &responses[rPtr], bytes_left);
    }

    int found_count = 0;
    for(int i = 0; i < 128 && found_count < max_devices; ++i)
    {
        if(responses[i] == ERROR_NONE)
        {
            found_addresses[found_count++] = i;
        }
        else if(responses[i] != ERROR_ADDRESS_NACK)
        {
            fprintf(OUTPUT_ERROR, "Unexpected error when scanning address %d: error code %d.\n", i, responses[i]);
        }
    }
    return found_count;
}

//------------------------------------------
// i2c_pololu_error_string()
//------------------------------------------
const char *i2c_pololu_error_string( int error_code )
{
    // Make sure we are looking at a positive error code
    if(error_code < 0)
    {
        error_code = -error_code;
    }
    switch(error_code)
    {
        case ERROR_NONE:
            return "No error";
        case ERROR_PROTOCOL:
            // This error code indicates that the command itself was invalid. Double check the bytes you are sending
            // if you get this error.
            return "Protocol error";
        case ERROR_PREVIOUS_TIMEOUT:
            return "Timeout from previous command";
        case ERROR_TIMEOUT:
            // This error code indicates that the command took longer than the configurable I²C timeout period, so it was aborted.
            // If this error or any of the other timeout errors occur, you might consider using the “Set I²C timeout” command to
            // raise the timeout, which is 50 ms by default.
            return "Timeout";
        case ERROR_ADDRESS_TIMEOUT:
            // This error code indicates that a timeout happened while transmitting the first byte of an I²C transfer,
            // which contains the device address.
            return "Timeout while sending address";
        case ERROR_TX_TIMEOUT:
            // This error code indicates that a timeout happened while transmitting a data byte during an I²C write.
            return "Timeout while transmitting";
        case ERROR_RX_TIMEOUT:
            // This error code indicates that a timeout happened while receiving a data byte during an I²C read.
            return "Timeout while receiving";
        case ERROR_NACK:
            // Generic NACK received during I²C transaction.
            return "Received NACK";
        case ERROR_ADDRESS_NACK:
            // This error code indicates that the adapter received a NACK (Not Acknowledge) while transmitting the
            // address byte to the I²C bus. This error can happen if the target device is not powered, if the target
            // device is not properly connected to the I²C bus, or—for adapters that do not provide power to the bus—if
            // the adapter’s VCC (IN) pin is not powered.
            return "Address NACK: target device did not respond";
        case ERROR_TX_DATA_NACK:
            // This error code indicates that the adapter received a NACK while transmitting a byte of data to the
            // target I²C device.
            return "Received NACK for TX data";
        case ERROR_BUS_ERROR:
            // This error code indicates that the adapter detected an unexpected START or STOP condition on the I²C bus
            return "Bus error";
        case ERROR_ARBITRATION_LOST:
            // This error code indicates that the adapter tried to send a high level on the I²C SDA line, but it detected a low level. This could happen if another I²C controller is connected to the same bus and initiates communication at the same time as the adapter.
            return "Arbitration lost";
        case ERROR_NOT_SUPPORTED:
            // This error code indicates that the adapter recognized the command but it does not support it (e.g. due
            // to lacking necessary hardware).
            return "Operation not supported";
        default:
            return "Unknown error";
    }
}

// ------------------------------------------
// i2c_pololu_is_device_valid()
// ------------------------------------------
int i2c_pololu_is_device_valid(const char* path)
{
    if(path == NULL || *path == '\0')
    {
        return -EINVAL;
    }

    // First, ensure the device node is present/available
    int rc = i2c_pololu_check_device_available(path, 500);
    if(rc != 0)
    {
        return rc; // propagate errno-style code
    }

    i2c_pololu_adapter tmp = { .fd = -1 };
    if(i2c_pololu_connect(&tmp, path) != 0)
    {
        return -EIO;
    }

    i2c_pololu_device_info info;
    rc = i2c_pololu_get_device_info(&tmp, &info);
    i2c_pololu_disconnect(&tmp);
    if(rc != 0)
    {
        return -EIO;
    }

    // Validate vendor/product IDs against known Pololu IDs
    // Pololu USB Vendor ID: 0x1FFB; Product IDs known: 0x2502, 0x2503
    if(info.vendor_id != 0x1FFB)
    {
        return -ENODEV;
    }
    if(!(info.product_id == 0x2502 || info.product_id == 0x2503))
    {
        return -ENODEV;
    }

    return 0;
}
