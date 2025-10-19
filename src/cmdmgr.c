//=========================================================================
// cmdmgr.c
//
// commandline / configuration management routines for runMag utility.
//
// Author:      David Witten, KD0EAG
// Date:        December 18, 2023
// Updates:     Sept. 9, 2025
//
// License:     GPL 3.0
// Notes:       USE_PIGPIO version replaces i2c.c (using file system calls
//              to read(), write(), etc.) with calls to pigpio.
//
//              USE_WAITFOREDGE waits for an edge interrupt on PI GPIO27
//              for PPS rising edge.
//
//              FOR_GRAPE2 uses pigpio daemon and pipes to communicate with
//              with a parent GRAPE2 Personal Space Weather Station setup.
//=========================================================================
#include <stdio.h>
#include "main.h"
#include "magdata.h"
#include "cmdmgr.h"

//extern char Version;

//------------------------------------------
// currentTimeMillis()
//------------------------------------------
long currentTimeMillis()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

//------------------------------------------
// showSettings()
//------------------------------------------
void showSettings(pList *p)
{
//    char pathStr[p->portpath] = "";

    fprintf(stdout, "\nVersion = %s\n", p->Version);
    fprintf(stdout, "\nCurrent Parameters:\n\n");
//#if (USE_PIPES)
    fprintf(stdout, "   Log output to pipes:                        %s\n",          p->usePipes ? "TRUE" : "FALSE");
    fprintf(stdout, "   Input file path:                            %s\n",          p->pipeInPath);
    fprintf(stdout, "   Output file path:                           %s\n",          p->pipeOutPath);
//#endif
#if(USE_POLOLU)
    fprintf(stdout, "   I2C bus adapter path:                 %s\n",          p->portpath);
#else
    snprintf(pathStr, sizeof(pathStr), "/dev/i2c-%i", p->i2cBusNumber);
    fprintf(stdout, "         I2C bus:                              %s (dec)\n",    pathstr);
#endif
    fprintf(stdout, "   Built in self test (BIST) value:      %02X (hex)\n",  p->doBistMask);
    fprintf(stdout, "   Device sampling mode:                 %s\n",          p->samplingMode     ? "CONTINUOUS" : "POLL");
    fprintf(stdout, "   Data ready delay value:               %i (dec)\n",    p->DRDYdelay);
    fprintf(stdout, "   Cycle counts by vector:               X: %3i (dec), Y: %3i (dec), Z: %3i (dec)\n", p->cc_x, p->cc_y, p->cc_z);
    fprintf(stdout, "   Gain by vector:                       X: %3i (dec), Y: %3i (dec), Z: %3i (dec)\n", p->x_gain, p->y_gain, p->z_gain);
    fprintf(stdout, "   Read back CC Regs after set:          %s\n",          p->readBackCCRegs   ? "TRUE" : "FALSE");
//    fprintf(stdout, "   CMM sample rate:                          %2X (hex)\n",   p->CMMSampleRate);
    fprintf(stdout, "   TMRC reg value:                       %2X (hex)\n",   p->TMRCRate);
    fprintf(stdout, "   Remote temperature address:           %02X (hex)\n",  p->remoteTempAddr);
    fprintf(stdout, "   Magnetometer address:                 %02X {hex)\n",  p->magAddr);
    fprintf(stdout, "\n\n");
}

//------------------------------------------
// getCommandLine()
//------------------------------------------
int getCommandLine(int argc, char** argv, pList *p)
{
    int c;

    while((c = getopt(argc, argv, "?B:c:CD:g:P:MSTV")) != -1)
    {
        //int this_option_optind = optind ? optind : 1;
        switch(c)
        {
            case 'B':
                p->doBistMask = atoi(optarg);
                break;
            case 'C':
                p->readBackCCRegs = TRUE;
                break;
            case 'c':
                //p->cc_x = p->cc_y = p->cc_z = atoi(optarg);
                p->cc_x = p->cc_y = p->cc_z = (int) strtol(optarg, NULL, 10);
                if(p->cc_x > 0x320 || p->cc_x <= 0)
                {
                    fprintf(stderr, "\n ERROR Invalid: cycle count > 800 (dec) or cycle count  <= 0.\n\n");
                    exit(1);
                }
                p->x_gain = p->y_gain = p->z_gain = getCCGainEquiv(p->cc_x);
                break;
            case 'D':
                //p->CMMSampleRate = atoi(optarg);
                p->CMMSampleRate = (int) strtol(optarg, NULL, 10);
                break;
            case 'g':
                //p->samplingMode = atoi(optarg);
                p->samplingMode = (int) strtol(optarg, NULL, 10);
                break;
            case 'P':
                strcpy(p->portpath, optarg);
                break;
            case 'M':
                p->checkMagSensor = TRUE;
                break;
            case 'S':
                p->scanI2CBUS = TRUE;
                break;
            case 'T':
                p->checkTempSensor = TRUE;
                break;
            case 'V':
                fprintf(stdout, "\nVersion: %s\n", p->Version);
                exit(0);
                break;
            case 'h':
            case '?':
                fprintf(stdout, "\nParameters:\n\n");
                fprintf(stdout, "   -B <reg mask>          :  Do built in self test (BIST).         [ Not implemented ]\n");
                fprintf(stdout, "   -C                     :  Read back cycle count registers before sampling.\n");
                fprintf(stdout, "   -c <count>             :  Set cycle counts as integer.          [ default: 200 decimal]\n");
                fprintf(stdout, "   -D <rate>              :  Set magnetometer sample rate.         [ TMRC reg 96 hex default ].\n");
                fprintf(stdout, "   -g <mode>              :  Device sampling mode.                 [ POLL=0 (default), CONTINUOUS=1 ]\n");
#if(USE_POLOLU)
                fprintf(stdout, "   -P                     :  Path to Pololu port in /dev.          [ default: /dev/ttyACM0 ]\n");
#endif
                fprintf(stdout, "   -M                     :  Verify Magnetometer presence and version.\n");
                fprintf(stdout, "   -S                     :  List devices seen on i2c bus and exit.\n");
                fprintf(stdout, "   -T                     :  Verify Temperature sensor presence and version.\n");
                fprintf(stdout, "   -V                     :  Display software version and exit.\n");
                fprintf(stdout, "   -h or -?               :  Display this help.\n\n");
                return 1;
                break;
            default:
                fprintf(stdout, "\n?? getopt returned character code 0x%2X ??\n", c);
                break;
        }
    }
    if(optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while(optind < argc)
        {
            printf("%s ", argv[optind++]);
        }
        printf("\n");
    }
    return 0;
}

