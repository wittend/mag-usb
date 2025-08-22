//=========================================================================
// rm3100.h
// 
// An interface for the RM3100 3-axis magnetometer from PNI Sensor Corp.
// Author:      David Witten, KD0EAG
// Date:        Dacember 18, 2023
// License:     GPL 3.0
// Note:        replaces i2c.c (using file system calls to read(), write(), etc.
//              with calls to pigpio. 
//              Also adding callbacks on GPIO27 for PPS rising edge.
//=========================================================================
#ifndef SWX3100DEFS_h
#define SWX3100DEFS_h

#define MAX_I2C_WRITE 32

#ifndef TRUE
#define TRUE        1
#endif

#ifndef FALSE
#define FALSE       0
#endif

#define LOCAL       0
#define REMOTE      1

//-------------------------------------------
// Enumerations 
//-------------------------------------------
typedef enum
{
    SensorPowerModePowerDown    = 0,       
    SensorPowerModeSuspend      = 1,       
    SensorPowerModeActive       = 255,  
} SensorPowerMode;
 
//-------------------------------------------
// SensorStatus
//-------------------------------------------
typedef enum
{
    /* Valid Responses */
    SensorOK,                       // Sensor responded with expected data.
    SensorInitialized,              // Sensor has been initialized.
 
    /* Error Responses */
    SensorUnknownError,             // An unknown error has occurred.
    SensorErrorNonExistant,         // Unable to communicate with sensor, sensor did not ACK.
    SensorErrorUnexpectedDevice,    // A different sensor was detected at the address.
 
    SensorStatusPending = 255,      // Reserved for internal used.
} SensorStatus;

//-------------------------------------------
// I2C bus to use
//-------------------------------------------
#define RASPI_I2C_BUS0      0
#define RASPI_I2C_BUS1      1

//-------------------------------------------
// Device  sampling mode
//-------------------------------------------
#define POLL                0
#define CONTINUOUS          1

//-------------------------------------------
// Continuous Measurement Mode
//-------------------------------------------
#define CMMMODE_START       1
#define CMMMODE_DRDM        4     // Dont Use
#define CMMMODE_CMX         16
#define CMMMODE_CMY         32
#define CMMMODE_CMZ         64

#define CMMMODE_ALL   (CMMMODE_START | CMMMODE_CMX | CMMMODE_CMY | CMMMODE_CMZ)

//-------------------------------------------
// Polled Measurement Mode
//-------------------------------------------
#define PMMODE_CMX          16
#define PMMODE_CMY          32
#define PMMODE_CMZ          64

#define PMMODE_ALL   (PMMODE_CMX | PMMODE_CMY | PMMODE_CMZ)

//-------------------------------------------
// I2C bus speed options available
//-------------------------------------------
// #define I2C_LOWSPEED        10000
#define I2C_STANDARD        100000
#define I2C_FASTMODE        1000000
#define I2C_HIGHSPEED       3400000

#define CCP0                0xC8        // 200 Cycle Count
#define CCP1                0x00

// #define NOS                 0x01        // Number of Samples for averaging

//-------------------------------------------
// Cycle Count values (16 bit)
//-------------------------------------------
#define CC_50               0x32        // 50  decimal
#define CC_100              0x64        // 100 decimal
#define CC_200              0xC8        // 200 decimal (default)
#define CC_300              0x12C       // 300 decimal
#define CC_400              0x190       // 400 decimal
#define CC_800              0x320       // 800 decimal

//-------------------------------------------
// Gain values (decimal)
//-------------------------------------------// 
#define GAIN_20             20
#define GAIN_38             38
#define GAIN_75             75
#define GAIN_113            113
#define GAIN_150            150
#define GAIN_300            300

//-------------------------------------------------------------------------------------------------
// CMM Update Rates
//
// Note: The Cycle Count Registers establish the maximum data rate of the sensors. For instance, if
// the cycle count is set to 200D, then the maximum single-axis update rate is ~430 Hz. If TMRC is
// set to 0x92, indicating an update rate of ~600 Hz, the rate established by the cycle count will
// override the TMRC request, and the actual update rate will be ~430 Hz.
//-------------------------------------------------------------------------------------------------
#define TMRC_VAL_600        0x92    // Time between readings: ~1.7 ms
#define TMRC_VAL_300        0x93    // Time between readings: ~3 ms
#define TMRC_VAL_150        0x94    // Time between readings: ~7 ms
#define TMRC_VAL_75         0x95    // Time between readings: ~13 ms
#define TMRC_VAL_37         0x96    // Time between readings: ~27 ms
#define TMRC_VAL_18         0x97    // Time between readings: ~55 ms
#define TMRC_VAL_9          0x98    // Time between readings: ~110 ms
#define TMRC_VAL_4p5        0x99    // Time between readings: ~220 ms
#define TMRC_VAL_2p3        0x9A    // Time between readings: ~440 ms
#define TMRC_VAL_1p2        0x9B    // Time between readings: ~0.8 s
#define TMRC_VAL_0p6        0x9C    // Time between readings: ~1.6 s
#define TMRC_VAL_0p3        0x9D    // Time between readings: ~3.3 s
#define TMRC_VAL_0p15       0x9E    // Time between readings: ~6.7 s
#define TMRC_VAL_0p07       0x9F    // Time between readings: ~13 s 
// Default rate 125 Hz 

//-------------------------------------------
// BIST bit positions.
//-------------------------------------------
#define BIST_BPO    0   // BIST LR Periods.
#define BIST_BP1    1   //   (BP0=1 & BP1=0) = 1 period; (BP0=0 & BP1=1) = 2 periods; (BP0=1 & BP1=1) = 4 periods.
#define BIST_BW0    2   // BW0 & BW1 Determine the BIST timeout [Sleep Oscillation Cycle (30 µs)].
#define BIST_BW1    3   //   (BW0=1 & BW1=0) = 30 uSec;  (BW0=0 & BW1=1) = 60 uSec; (BW0=1 & BW1=1) = 120 uSec.
#define BIST_XOK    4   // Read only. Valid only when STE = 1 
#define BIST_YOK    5   // Read only. Valid only when STE = 1 
#define BIST_ZOK    6   // Read only. Valid only when STE = 1 
#define BIST_STE    7   // Setting this to 1 commands the RM3100 Testing Boards to run the builtin self-test when
                        //    the POLL register is written to. The end of the built-in self-test sequence will be
                        //    indicated by DRDY going HIGH.

//-------------------------------------------
// Address of the device
//-------------------------------------------
#define RM3100_I2C_ADDRESS              0x20
#define RM3100_I2C_ADDRESS_7bit         0x20
#define RM3100_I2C_ADDRESS_8bit         0x20 << 1 // MBED uses 8 bit address

//-------------------------------------------
// Value of the MagI2C Version expected
//-------------------------------------------
#define RM3100_VER_EXPECTED     0x22

//-------------------------------------------
//  From:   Table 5-1: MagI2C Register Map
//          "RM3100 & RM2100 Sensor Suite User Manual"
//          PNI Sensor Corporation
//          Doc 1017252 R07
//          Page 28 of 43
//-------------------------------------------
//  Name  |  Register #(Hex) | R/W      | Default (Hex) | Payload Format | Description
//-------------------------------------------------------------------------------
// POLL   |         00       |  RW      | 00            |     [7:0]      | Polls for a Single Measurement (aka MAG)
#define RM3100_MAG_POLL     0x00

// CMM    |         01       |  RW      | 00            |     [7:0]      | Initiates Continuous Measurement Mode (aka 'Beacon')
#define RM3100I2C_CMM       0x01

// CCX    |      04 – 05**< @brief     |  RW      | 00C8          |    UInt16      | Cycle Count Register – X Axis
// Set values with  i2cset:  "sudo i2cset -y 1 0x20 0x04 0x01 0x90 0x01 0x90 0x01 0x90 0x0A i"
// Read values with i2cdump: "sudo i2cdump -r 0x04-0x0a 2 0x20"
#define RM3100I2C_CCX_1     0x04
#define RM3100I2C_CCX_0     0x05

// CCY    |      06 – 07     |  RW      | 00C8          |    UInt16      | Cycle Count Register – Y Axis
#define RM3100I2C_CCY_1     0x06
#define RM3100I2C_CCY_0     0x07

// CCZ    |      08 – 09     |  RW      | 00C8          |    Uint16      | Cycle Count Register – Z Axis
#define RM3100I2C_CCZ_1     0x08
#define RM3100I2C_CCZ_0     0x09

// NOS ? - taken from mBed code but not currently understood. May mean "Number of Samples for averaging".
#define RM3100I2C_NOS       0x0A

// TMRC   |      0B          |  RW      | 96            |     [7:0]      | Sets Continuous Measurement Mode Data Rate
#define RM3100I2C_TMRC      0x0B

// MX     |      24 – 26     |  R       | 000000        |    Uint24      | Measurement Results – X Axis
#define RM3100I2C_XYZ       0x24

#define RM3100I2C_MX        0x24
#define RM3100I2C_MX_2      0x24
#define RM3100I2C_MX_1      0x25
#define RM3100I2C_MX_0      0x26

// MY     |      27 – 29     |  R       | 000000        |    Uint24      | Measurement Results – Y Axis
#define RM3100I2C_MY        0x27
#define RM3100I2C_MY_2      0x27
#define RM3100I2C_MY_1      0x28
#define RM3100I2C_MY_0      0x29

// MZ     |      2A – 2C     |  R       | 000000        |    Uint24      | Measurement Results – Z Axis
#define RM3100I2C_MZ        0x2A
#define RM3100I2C_MZ_2      0x2A
#define RM3100I2C_MZ_1      0x2B
#define RM3100I2C_MZ_0      0x2C

// BIST   |      33          |  RW      | 00            |    [7:0]       | Built-In Self Test
#define RM3100I2C_BIST_WR   0x33

// STATUS |      34          |  R       | 00            |    [7:0]       | Status of DRDY
#define RM3100I2C_STATUS    0x34

// HSHAKE |      35          |  RW      | 1B            |    [7:0]       | Handshake Register
#define RM3100I2C_HSHAKE    0x35

// REVID  |      36          |  R       |       --      |    Unit8       | MagI2C Revision Identification
#define RM3100I2C_REVID     0x36

#define RM3100I2C_READMASK  0x80

#define RM3100I2C_POLLX     0x10
#define RM3100I2C_POLLY     0x20
#define RM3100I2C_POLLZ     0x40  
#define RM3100I2C_POLLXYZ   0x70        // POLLX & POLLY && POLLZ

#define CALIBRATION_TIMEOUT 5000        // timeout in milliseconds
#define DEG_PER_RAD (180.0/3.14159265358979)

// Regs and Values taken from mBed code but not currently understood.
//--------------------------------------------------------------------
#define RM3100I2C_ENABLED           0x79
#define RM3100I2C_DISABLED          0x00

#define RM3100_TEST3_REG            0x72
#define RM3100_LROSCADJ_REG         0x63

#define RM3100_LROSCADJ_VALUE       0xA7
#define RM3100_SLPOSCADJ_VALUE      0x08 

#endif  //SWX3100DEFS_h
