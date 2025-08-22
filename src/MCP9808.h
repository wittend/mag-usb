//=========================================================================
// MCP9808.h
// 
// An interface for the MCP9808 precision temperature sensor from Microchip.
//
// Author:      David Witten, KD0EAG
// Date:        May 2, 2020
// License:     GPL 3.0
//=========================================================================
#ifndef MCP9808_H
#define MCP9808_H
//-----------------------------------
// MCP9808 Bus Address.
//-----------------------------------
#define MCP9808_LCL_I2CADDR_DEFAULT         0x18       // alt: (selected by chip hardware 0x18 - 0x1F
#define MCP9808_RMT_I2CADDR_DEFAULT         0x19       // alt: (selected by chip hardware 0x18 - 0x1F

// MCP9808 Registers
//-----------------------------------
#define MCP9808_REG_CONFIG              0x01
#define MCP9808_REG_UPPER_TEMP          0x02
#define MCP9808_REG_LOWER_TEMP          0x03
#define MCP9808_REG_CRIT_TEMP           0x04
#define MCP9808_REG_AMBIENT_TEMP        0x05
#define MCP9808_REG_MANUF_ID            0x06
#define MCP9808_REG_DEVICE_ID           0x07
#define MCP9808_REG_RESOLUTION          0x08

// Configuration register values.
//-----------------------------------
#define MCP9808_REG_CONFIG_SHUTDOWN     0x0100
#define MCP9808_REG_CONFIG_CRITLOCKED   0x0080
#define MCP9808_REG_CONFIG_WINLOCKED    0x0040
#define MCP9808_REG_CONFIG_INTCLR       0x0020
#define MCP9808_REG_CONFIG_ALERTSTAT    0x0010
#define MCP9808_REG_CONFIG_ALERTCTRL    0x0008
#define MCP9808_REG_CONFIG_ALERTSEL     0x0004
#define MCP9808_REG_CONFIG_ALERTPOL     0x0002
#define MCP9808_REG_CONFIG_ALERTMODE    0x0001

// Expected return values.
//-----------------------------------
#define MCP9808_MANID_EXPECTED          0x0054
#define MCP9808_DEVREV_EXPECTED         0x0400

#endif //MCP9808_H
