//=========================================================================
// i2c_pololu.c
//
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
//
// Author:      David Witten, KD0EAG
// Date:        7/17/25
// License:     GPL 3.0
//=========================================================================
#ifndef POLOLU_I2C_H
#define POLOLU_I2C_H

#include <stdint.h>
#include <stdbool.h>

// Command constants                // taken from protocol.h of Pololu firmware v. 1.01.
#define CMD_I2C_WRITE 0x91
#define CMD_I2C_READ 0x92
#define CMD_SET_I2C_MODE 0x94
#define CMD_SET_I2C_TIMEOUT 0x97
#define CMD_CLEAR_BUS 0x98
#define CMD_I2C_WRITE_AND_READ 0x9B
#define CMD_SET_STM32_TIMING 0xA1
#define CMD_DIGITAL_READ 0xA2
#define CMD_ENABLE_VCC_OUT 0xA4
#define CMD_GET_DEVICE_INFO 0xA7
#define CMD_GET_DEBUG_DATA 0xDC

// Error Codes
#define ERROR_NONE 0
#define ERROR_PROTOCOL 1
#define ERROR_PREVIOUS_TIMEOUT 2
#define ERROR_TIMEOUT 3
#define ERROR_ADDRESS_TIMEOUT 4
#define ERROR_TX_TIMEOUT 5
#define ERROR_RX_TIMEOUT 6
#define ERROR_NACK 7
#define ERROR_ADDRESS_NACK 8
#define ERROR_TX_DATA_NACK 9
#define ERROR_BUS_ERROR 10
#define ERROR_ARBITRATION_LOST 11
#define ERROR_OTHER 12
#define ERROR_NOT_SUPPORTED 13

// I2C Modes
#define I2C_STANDARD_MODE 0
#define I2C_FAST_MODE 1
#define I2C_FAST_MODE_PLUS 2
#define I2C_10_KHZ 3

#define OUTPUT_PRINT        stdout
#define OUTPUT_ERROR        stderr

// Represents a connection to a Pololu Isolated USB-to-I2C Adapter.
typedef struct
{
    int fd; // File descriptor for the serial port
} i2c_pololu_adapter;

/**
 * @brief Check that a device node exists and is not exclusively held by another process.
 *        Intended for Linux /dev ttyACM/ttyUSB style devices prior to opening.
 *
 * This function will:
 *  - Poll for the existence of the path until timeout_ms expires.
 *  - Verify the path is a character device.
 *  - Attempt a non-blocking open(O_RDWR|O_NOCTTY|O_NONBLOCK). If it succeeds, the
 *    descriptor is closed immediately and the device is considered available.
 *    If the driver has exclusive-use set (e.g., via TIOCEXCL by another process),
 *    the open will typically fail with EBUSY.
 *
 * @param path       Device node path under /dev (e.g., "/dev/ttyACM0").
 * @param timeout_ms How long to wait for the device to appear/become free. Use 0 for a single check.
 * @return 0 if available.
 *         Negative errno-style value on failure: -ENOENT, -ENOTTY/-ENODEV if not character device,
 *         -EBUSY if in use, or -EACCES/-EPERM on permission error, etc.
 */
int i2c_pololu_check_device_available(const char* path, int timeout_ms);

// Structure to hold device information
typedef struct
{
    uint16_t vendor_id;
    uint16_t product_id;
    char firmware_version[16];
    uint16_t firmware_version_bcd;
    char firmware_modification[9];
    // 6 groups of 2 bytes formatted as "%02X%02X-" (5 chars per group)
    // results in 6*5 = 30 characters including trailing dash; we'll null-terminate after replacing the last dash.
    char serial_number[30];
} i2c_pololu_device_info;

// Function Prototypes

/**
 * @brief Initializes the adapter structure.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 */
int i2c_pololu_init( i2c_pololu_adapter *adapter );

/**
 * @brief Connects the adapter to the specified serial port.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param port_name The name of the serial port (e.g., "/dev/ttyACM0").
 * @return 0 on success, -1 on failure.
 */
int i2c_pololu_connect( i2c_pololu_adapter *adapter, const char *port_name );

/**
 * @brief Disconnects the adapter from the serial port.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 */
void i2c_pololu_disconnect( i2c_pololu_adapter *adapter );

/**
 * @brief Checks if the adapter is connected.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @return True if connected, false otherwise.
 */
bool i2c_pololu_is_connected( const i2c_pololu_adapter *adapter );

/**
 * @brief Writes data to an I2C target.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param address The 7-bit I2C address.
 * @param reg, register to write The 7-bit I2C address.
 * @param data A pointer to the data to write.
 * @param size The number of bytes to write.
 * @return The number of bytes written, or a negative error code on failure.
 */
int i2c_pololu_write_to( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, const uint8_t *data, uint8_t size );

/**
 * @brief Reads data from an I2C target.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param address The 7-bit I2C address.
 * @param data A buffer to store the read data.
 * @param size The number of bytes to read.
 * @return The number of bytes read, or a negative error code on failure.
 */
int i2c_pololu_read_from( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, uint8_t *data, uint8_t size );

/**
 * @brief Reads data from an I2C target.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param address The 7-bit I2C address.
 * @param data A buffer to store the read data.
 * @param size The number of bytes to read.
 * @return The number of bytes read, or a negative error code on failure.
 */
int i2c_pololu_write_and_read_from( i2c_pololu_adapter *adapter, uint8_t address, uint8_t reg, uint8_t *data, uint8_t size );

/**
 * @brief Sets the I2C frequency.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param frequency_khz The desired frequency in kHz.
 * @return 0 on success, negative error code on failure.
 */
int i2c_pololu_set_frequency( i2c_pololu_adapter *adapter, unsigned int frequency_khz );

/**
 * @brief Clears the I2C bus.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @return 0 on success, negative error code on failure.
 */
int i2c_pololu_clear_bus( i2c_pololu_adapter *adapter );

/**
 * @brief Gets information about the device.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param info A pointer to a i2c_pololu_device_info struct to be filled.
 * @return 0 on success, negative error code on failure.
 */
int i2c_pololu_get_device_info( i2c_pololu_adapter *adapter, i2c_pololu_device_info *info );

/**
 * @brief Scans the I2C bus for devices.
 * @param adapter A pointer to the i2c_pololu_adapter struct.
 * @param found_addresses An array to store the addresses of found devices.
 * @param max_devices The maximum number of devices to find (size of the array).
 * @return The number of devices found, or a negative error code on failure.
 */
int i2c_pololu_scan( i2c_pololu_adapter *adapter, uint8_t *found_addresses, int max_devices );

/**
 * @brief Sets the I²C mode.
 * @param mode one of four supported I²C modes
 *      0: Standard-mode (100 kHz)
 *      1: Fast-mode (400 kHz)
 *      2: Fast-mode Plus (1000 kHz)
 *      3: 10 kHz mode
 */
int i2c_pololu_set_i2c_mode(int mode);

/**
 * @brief Set I²C timeout
 * @return
 */
int i2c_pololu_set_i2c_timeout();

/*
 * @brief Set STM32 timing.
 * @return
 */
int i2c_pololu_set_STM32_timing();

/**
 * @brief Digital read.
 * @return
 */
int i2c_pololu_digital_read();

/**
 * @brief Enable VCC Out.
 * @return
 */
int i2c_pololu_enable_VCC_out();

/**
 * @brief Returns a string description for an error code.
 * @param error_code The error code.
 * @return A constant string describing the error.
 */
const char *i2c_pololu_error_string( int error_code );

/**
 * @brief Validate that the device at the given path is a supported Pololu I2C adapter.
 *        Connects, reads device info, and checks vendor/product IDs.
 * @param path Device node path under /dev (e.g., "/dev/ttyACM0").
 * @return 0 if valid; negative errno-style code on error or mismatch (e.g., -ENODEV).
 */
int i2c_pololu_is_device_valid(const char* path);

#endif // POLOLU_I2C_H
