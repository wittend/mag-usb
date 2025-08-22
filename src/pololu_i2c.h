//
// Created by dave on 7/17/25.
//

#ifndef POLOLU_I2C_H
#define POLOLU_I2C_H

#include <stdint.h>
#include <stdbool.h>

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

// Represents a connection to a Pololu Isolated USB-to-I2C Adapter.
typedef struct
{
    int fd; // File descriptor for the serial port
} pololu_i2c_adapter;

// Structure to hold device information
typedef struct
{
    uint16_t vendor_id;
    uint16_t product_id;
    char firmware_version[16];
    uint16_t firmware_version_bcd;
    char firmware_modification[9];
    char serial_number[25];
} pololu_i2c_device_info;

// Function Prototypes

/**
 * @brief Initializes the adapter structure.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 */
void pololu_i2c_init( pololu_i2c_adapter *adapter );

/**
 * @brief Connects the adapter to the specified serial port.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param port_name The name of the serial port (e.g., "/dev/ttyACM0").
 * @return 0 on success, -1 on failure.
 */
int pololu_i2c_connect( pololu_i2c_adapter *adapter, const char *port_name );

/**
 * @brief Disconnects the adapter from the serial port.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 */
void pololu_i2c_disconnect( pololu_i2c_adapter *adapter );

/**
 * @brief Checks if the adapter is connected.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @return True if connected, false otherwise.
 */
bool pololu_i2c_is_connected( const pololu_i2c_adapter *adapter );

/**
 * @brief Writes data to an I2C target.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param address The 7-bit I2C address.
 * @param data A pointer to the data to write.
 * @param size The number of bytes to write.
 * @return The number of bytes written, or a negative error code on failure.
 */
int pololu_i2c_write_to( pololu_i2c_adapter *adapter, uint8_t address, const uint8_t *data, uint8_t size );

/**
 * @brief Reads data from an I2C target.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param address The 7-bit I2C address.
 * @param data A buffer to store the read data.
 * @param size The number of bytes to read.
 * @return The number of bytes read, or a negative error code on failure.
 */
int pololu_i2c_read_from( pololu_i2c_adapter *adapter, uint8_t address, uint8_t *data, uint8_t size );

/**
 * @brief Sets the I2C frequency.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param frequency_khz The desired frequency in kHz.
 * @return 0 on success, negative error code on failure.
 */
int pololu_i2c_set_frequency( pololu_i2c_adapter *adapter, unsigned int frequency_khz );

/**
 * @brief Clears the I2C bus.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @return 0 on success, negative error code on failure.
 */
int pololu_i2c_clear_bus( pololu_i2c_adapter *adapter );

/**
 * @brief Gets information about the device.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param info A pointer to a pololu_i2c_device_info struct to be filled.
 * @return 0 on success, negative error code on failure.
 */
int pololu_i2c_get_device_info( pololu_i2c_adapter *adapter, pololu_i2c_device_info *info );

/**
 * @brief Scans the I2C bus for devices.
 * @param adapter A pointer to the pololu_i2c_adapter struct.
 * @param found_addresses An array to store the addresses of found devices.
 * @param max_devices The maximum number of devices to find (size of the array).
 * @return The number of devices found, or a negative error code on failure.
 */
int pololu_i2c_scan( pololu_i2c_adapter *adapter, uint8_t *found_addresses, int max_devices );

/**
 * @brief Sets the I²C mode.
 * @param mode one of four supported I²C modes
 *      0: Standard-mode (100 kHz)
 *      1: Fast-mode (400 kHz)
 *      2: Fast-mode Plus (1000 kHz)
 *      3: 10 kHz mode
 */
int pololu_set_i2c_mode(int mode);

/**
 * @brief Set I²C timeout
 * @return
 */
int pololu_set_i2c_timeout();

/*
 * @brief Set STM32 timing.
 * @return
 */
int pololu_set_STM32_timing();

/**
 * @brief Digital read.
 * @return
 */
int pololu_digital_read();

/**
 * @brief Enable VCC Out.
 * @return
 */
int pololu_enable_VCC_out();

/**
 * @brief Returns a string description for an error code.
 * @param error_code The error code.
 * @return A constant string describing the error.
 */
const char *pololu_i2c_error_string( int error_code );

#endif // POLOLU_I2C_H
