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
//=========================================================================
#include <stdio.h>
#include "main.h"
#include "magdata.h"
#include "cmdmgr.h"

//------------------------------------------
// showSettings()
//------------------------------------------
void showSettings(pList *p)
{
    fprintf(stdout, "\nVersion = %s\n", p->Version ? p->Version : "");
    fprintf(stdout, "\nCurrent Parameters:\n\n");

#if(USE_POLOLU)
    fprintf(stdout, "   Use external USB->I2C (Pololu):       %s\n",  p->use_I2C_converter ? "TRUE" : "FALSE");
    fprintf(stdout, "   I2C adapter device path:              %s\n",  p->portpath ? p->portpath : "(null)");
#else
    fprintf(stdout, "   Linux I2C bus number:                 %d\n",  p->i2cBusNumber);
#endif
    fprintf(stdout, "   Scan I2C bus on startup:              %s\n",  p->scanI2CBUS ? "TRUE" : "FALSE");
    fprintf(stdout, "   Verify Pololu adaptor:                %s\n",  p->checkPololuAdaptor ? "TRUE" : "FALSE");
    fprintf(stdout, "   Verify Temp sensor:                   %s\n",  p->checkTempSensor ? "TRUE" : "FALSE");
    fprintf(stdout, "   Verify Mag sensor:                    %s\n",  p->checkMagSensor ? "TRUE" : "FALSE");

    // Output/logging
    fprintf(stdout, "   Write logs:                           %s\n",  p->write_logs ? "TRUE" : "FALSE");
    fprintf(stdout, "   Log output path:                      %s\n",  p->log_output_path ? p->log_output_path : "(null)");
    fprintf(stdout, "   Create log path if empty:             %s\n",  p->create_log_path_if_empty ? "TRUE" : "FALSE");
    fprintf(stdout, "   Use named pipes:                      %s\n",  p->usePipes ? "TRUE" : "FALSE");
    fprintf(stdout, "   Pipe IN path:                         %s\n",  p->pipeInPath ? p->pipeInPath : "(null)");
    fprintf(stdout, "   Pipe OUT path:                        %s\n",  p->pipeOutPath ? p->pipeOutPath : "(null)");

    // Node info
    fprintf(stdout, "   Maintainer:                           %s\n",  p->maintainer ? p->maintainer : "(null)");
    fprintf(stdout, "   Maintainer email:                     %s\n",  p->maintainer_email ? p->maintainer_email : "(null)");

    // Location
    fprintf(stdout, "   Latitude:                             %s\n",  p->latitude ? p->latitude : "(null)");
    fprintf(stdout, "   Longitude:                            %s\n",  p->longitude ? p->longitude : "(null)");
    fprintf(stdout, "   Elevation:                            %s\n",  p->elevation ? p->elevation : "(null)");
    fprintf(stdout, "   Grid square:                          %s\n",  p->grid_square ? p->grid_square : "(null)");

    // Magnetometer
    fprintf(stdout, "   Magnetometer I2C address:             0x%02X (hex)\n",  (unsigned)(p->magAddr & 0xFF));
    fprintf(stdout, "   Cycle counts (X,Y,Z):                 %d, %d, %d\n",  p->cc_x, p->cc_y, p->cc_z);
    fprintf(stdout, "   Gains (X,Y,Z):                        %d, %d, %d\n",  p->x_gain, p->y_gain, p->z_gain);
    fprintf(stdout, "   TMRC register value:                  0x%02X (hex)\n",  (unsigned)(p->TMRCRate & 0xFF));
    fprintf(stdout, "   NOS register value:                   %d\n",  p->NOSRegValue);
    fprintf(stdout, "   DRDY delay (us):                      %d\n",  p->DRDYdelay);
    fprintf(stdout, "   Sampling mode:                        %s\n",  (p->samplingMode == CMM) ? "CMM" : "POLL");
    fprintf(stdout, "   CMM sample rate (Hz):                 %d\n",  p->CMMSampleRate);
    fprintf(stdout, "   Read back CC registers:               %s\n",  p->readBackCCRegs ? "TRUE" : "FALSE");
    fprintf(stdout, "   Orientation translate (deg XYZ):      %d, %d, %d\n",  p->mag_translate_x, p->mag_translate_y, p->mag_translate_z);

    // Temperature
    fprintf(stdout, "   Remote temperature I2C address:       0x%02X (hex)\n",  (unsigned)(p->remoteTempAddr & 0xFF));

    fprintf(stdout, "\n");
}

//------------------------------------------
// getCommandLine()
//------------------------------------------
int getCommandLine(int argc, char** argv, pList *p)
{
    int c;

    while((c = getopt(argc, argv, "h?B:c:CD:g:PMSQTVO:")) != -1)
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
                p->cc_x = p->cc_y = p->cc_z = (int) strtol(optarg, NULL, 10);
                if(p->cc_x > 0x320 || p->cc_x <= 0)
                {
                    fprintf(stderr, "\n ERROR Invalid: cycle count > 800 (dec) or cycle count  <= 0.\n\n");
                    exit(1);
                }
                p->x_gain = p->y_gain = p->z_gain = getCCGainEquiv(p->cc_x);
                break;
            case 'D':
                p->CMMSampleRate = (int) strtol(optarg, NULL, 10);
                break;
            case 'g':
                p->samplingMode = (int) strtol(optarg, NULL, 10);
                break;
            case 'O':
                if(p->portpath)
                {
                    snprintf(p->portpath, MAXPATHBUFLEN, "%s", optarg);
                }
                break;
            case 'P':
                p->showSettingsOnly = TRUE;
                break;
            case 'Q':
                p->checkPololuAdaptor = TRUE;
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
            case 'h':
            case '?':
                fprintf(stdout, "\nParameters:\n\n");
                fprintf(stdout, "   -B <reg mask>          :  Do built in self test (BIST).         [ Not implemented ]\n");
                fprintf(stdout, "   -C                     :  Read back cycle count registers before sampling.\n");
                fprintf(stdout, "   -c <count>             :  Set cycle counts as integer.          [ default: 200 decimal]\n");
                fprintf(stdout, "   -D <rate>              :  Set magnetometer sample rate.         [ TMRC reg 96 hex default ].\n");
                fprintf(stdout, "   -g <mode>              :  Device sampling mode.                 [ POLL=0 (default), CONTINUOUS=1 ]\n");
#if(USE_POLOLU)
                fprintf(stdout, "   -P                     :  Show all current settings and exit.\n");
                fprintf(stdout, "   -O                     :  Path to Pololu port in /dev.          [ default: /dev/ttyACM0 ]\n");
                fprintf(stdout, "   -Q                     :  Verify presence of Pololu adaptor.\n");
#endif
                fprintf(stdout, "   -M                     :  Verify Magnetometer presence and version.\n");
                fprintf(stdout, "   -S                     :  List devices seen on i2c bus and exit.\n");
                fprintf(stdout, "   -T                     :  Verify Temperature sensor presence and version.\n");
                fprintf(stdout, "   -V                     :  Display software version and exit.\n");
                fprintf(stdout, "   -h or -?               :  Display this help.\n\n");
                exit(0);
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

