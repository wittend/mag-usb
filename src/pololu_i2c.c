//
// Created by dave on 7/17/25.
//
#include "pololu_i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// Helper function to check for response errors
static int check_response( const uint8_t *response, size_t expected_len, size_t actual_len )
{
    if(actual_len < expected_len)
    {
        fprintf(stderr, "Timeout while reading response from adapter (received %zu bytes).\n", actual_len);
        return -ERROR_TIMEOUT;
    }
    if(response[0] != ERROR_NONE)
    {
        return -response[0];
    }
    return 0; // Success
}

void pololu_i2c_init( pololu_i2c_adapter *adapter )
{
    if(adapter)
    {
        adapter->fd = -1;
    }
}

int pololu_i2c_connect( pololu_i2c_adapter *adapter, const char *port_name )
{
    if(!adapter || !port_name)
    {
        return -1;
    }

    adapter->fd = open(port_name, O_RDWR | O_NOCTTY);
    if(adapter->fd < 0)
    {
        perror("Error opening serial port");
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

void pololu_i2c_disconnect( pololu_i2c_adapter *adapter )
{
    if(adapter && adapter->fd >= 0)
    {
        close(adapter->fd);
        adapter->fd = -1;
    }
}

bool pololu_i2c_is_connected( const pololu_i2c_adapter *adapter )
{
    return adapter && adapter->fd >= 0;
}

int pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t size )
{
    if(!pololu_i2c_is_connected(adapter))
    {
        return -1;
    }
    if(size > 255)
    {
        return -1;
    }

    uint8_t cmd[258];
    cmd[0] = 0x91;
    cmd[1] = address;
    cmd[2] = size;
    memcpy(&cmd[3], data, size);
    if(write(adapter->fd, cmd, size + 3) != size + 3)
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

int pololu_i2c_read_from( pololu_i2c_adapter *adapter, uint8_t address, uint8_t *data, uint8_t size )
{
    if(!pololu_i2c_is_connected(adapter))
    {
        return -1;
    }
    if(size > 255)
    {
        return -1;
    }

    uint8_t cmd[] = { 0x92, address, size };
    if(write(adapter->fd, cmd, 3) != 3)
    {
        perror("Failed to write to adapter");
        return -1;
    }

    uint8_t response[256];
    ssize_t len = read(adapter->fd, response, 1 + size);
    int error = check_response(response, 1 + size, len);
    if(error)
    {
        return error;
    }
    memcpy(data, &response[1], size);
    return size;
}

// Set I²C mode
int pololu_set_i2c_mode(int mode)
{
    return 0;
}

// Set I²C timeout
int pololu_set_i2c_timeout()
{
    return 0;
}

// Set STM32 timing
int pololu_set_STM32_timing()
{
    return 0;
}

// Digital read
int pololu_digital_read()
{
    return 0;
}

// Enable VCC Out
int pololu_enable_VCC_out()
{
    return 0;
}

// Get device info
int pololu_get_device_info()
{
    return 0;
}

int pololu_i2c_set_frequency( pololu_i2c_adapter *adapter, unsigned int frequency_khz )
{
    if(!pololu_i2c_is_connected(adapter))
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

    uint8_t cmd[] = { 0x94, mode };
    if(write(adapter->fd, cmd, 2) != 2)
    {
        perror("Failed to set frequency");
        return -1;
    }
    return 0;
}

int pololu_i2c_clear_bus( pololu_i2c_adapter *adapter )
{
    if(!pololu_i2c_is_connected(adapter))
    {
        return -1;
    }
    uint8_t cmd = 0x98;
    if(write(adapter->fd, &cmd, 1) != 1)
    {
        perror("Failed to clear bus");
        return -1;
    }
    return 0;
}

int pololu_i2c_get_device_info( pololu_i2c_adapter *adapter, pololu_i2c_device_info *info )
{
    if(!pololu_i2c_is_connected(adapter) || !info)
    {
        return -1;
    }
    uint8_t cmd = 0xA7;
    if(write(adapter->fd, &cmd, 1) != 1)
    {
        perror("Failed to request device info");
        return -1;
    }

    uint8_t length;
    if(read(adapter->fd, &length, 1) != 1)
    {
        perror("Failed to read device info length");
        return -1;
    }

    uint8_t raw_info[length];
    raw_info[0] = length;
    if(read(adapter->fd, &raw_info[1], length - 1) != length - 1)
    {
        perror("Failed to read device info payload");
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
        fprintf(stderr, "Unrecognized device info version: %d\n", raw->version);
        return -1;
    }

    info->vendor_id = raw->vendor_id;
    info->product_id = raw->product_id;
    info->firmware_version_bcd = raw->firmware_version_bcd;

    snprintf(info->firmware_version, sizeof(info->firmware_version), "%x.%02x", (raw->firmware_version_bcd >> 8), (raw->firmware_version_bcd & 0xFF));

    strncpy(info->firmware_modification, raw->firmware_modification, 8);
    info->firmware_modification[8] = '\0';
    if(strcmp(info->firmware_modification, "-") == 0)
    {
        info->firmware_modification[0] = '\0';
    }

    // Format the serial number
    for(int i = 0; i < 6; ++i)
    {
        sprintf(info->serial_number + i * 5, "%02X%02X-", (uint8_t) raw->serial_number[i * 2], (uint8_t) raw->serial_number[i * 2 + 1]);
    }
    info->serial_number[23] = '\0'; // Remove trailing dash

    return 0;
}

int pololu_i2c_scan( pololu_i2c_adapter *adapter, uint8_t *found_addresses, int max_devices )
{
    if(!pololu_i2c_is_connected(adapter) || !found_addresses || max_devices <= 0)
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
            fprintf(stderr, "Unexpected error when scanning address %d: error code %d.\n", i, responses[i]);
        }
    }
    return found_count;
}

const char *pololu_i2c_error_string( int error_code )
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
        case ERROR_ADDRESS_NACK:
            // This error code indicates that the adapter received a NACK (Not Acknowledge) while transmitting the
            // address byte to the I²C bus. This error can happen if the target device is not powered, if the target
            // device is not properly connected to the I²C bus, or—for adapters that do not provide power to the bus—if
            // the adapter’s VCC (IN) pin is not powered.
            return "Target device did not respond";
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
