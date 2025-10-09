## Me:
I want to add the ability to read and write configuration values to a TOML file named 'config.toml' that allows this program to persist configurations between startups.  Parameters received from the command line should override these settings, but should not persist between runs. I would prefer to avoid using an external library if possible, but a header-only library is OK.

## AI Chat:
I'll help you add TOML configuration support to your project. Since you prefer to avoid external libraries but are open to header-only solutions, I recommend using **tomlc99** or **toml++**, but given your project is in C, the best lightweight option is **toml.c** by cktan.

However, since you want to avoid dependencies if possible, let me provide a **simple custom TOML parser** that handles basic key-value pairs suitable for your configuration needs. This will be header-only and tailored to your `pList` structure.

Here's my solution:

## Step 1: Create a simple TOML configuration handler

```c++
//=========================================================================
// config_toml.h
//
// Simple TOML configuration file reader/writer for mag-usb
// Handles basic key=value pairs for configuration persistence
//
// License:     GPL 3.0
//=========================================================================
#ifndef CONFIG_TOML_H
#define CONFIG_TOML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CONFIG_LINE_MAX 256
#define CONFIG_KEY_MAX 64
#define CONFIG_VALUE_MAX 192

// Configuration structure matching pList parameters
typedef struct {
    // I2C Configuration
    int i2cBusNumber;
    int magAddr;
    int localTempAddr;
    int remoteTempAddr;
    
    // Magnetometer Configuration
    int cc_x;
    int cc_y;
    int cc_z;
    int x_gain;
    int y_gain;
    int z_gain;
    
    // Timing and Sampling
    int TMRCRate;
    int CMMSampleRate;
    int samplingMode;
    int NOSRegValue;
    int DRDYdelay;
    
    // Options
    int scanI2CBUS;
    int doBistMask;
    int readBackCCRegs;
    int tsMilliseconds;
    int usePipes;
    
    // Paths
    char portpath[CONFIG_VALUE_MAX];
    char pipeInPath[CONFIG_VALUE_MAX];
    char pipeOutPath[CONFIG_VALUE_MAX];
} Config;

// Initialize config with default values
static void config_init_defaults(Config *cfg) {
    if (!cfg) return;
    
    // I2C defaults
    cfg->i2cBusNumber = 1;
    cfg->magAddr = 0x20;
    cfg->localTempAddr = 0x18;
    cfg->remoteTempAddr = 0x19;
    
    // Magnetometer defaults
    cfg->cc_x = 200;
    cfg->cc_y = 200;
    cfg->cc_z = 200;
    cfg->x_gain = 1;
    cfg->y_gain = 1;
    cfg->z_gain = 1;
    
    // Timing defaults
    cfg->TMRCRate = 0;
    cfg->CMMSampleRate = 1;
    cfg->samplingMode = 0;
    cfg->NOSRegValue = 0;
    cfg->DRDYdelay = 100;
    
    // Options defaults
    cfg->scanI2CBUS = 0;
    cfg->doBistMask = 0;
    cfg->readBackCCRegs = 0;
    cfg->tsMilliseconds = 0;
    cfg->usePipes = 0;
    
    // Paths defaults
    strncpy(cfg->portpath, "/dev/i2c-1", CONFIG_VALUE_MAX - 1);
    cfg->portpath[CONFIG_VALUE_MAX - 1] = '\0';
    cfg->pipeInPath[0] = '\0';
    cfg->pipeOutPath[0] = '\0';
}

// Trim whitespace from string
static char* trim_whitespace(char *str) {
    char *end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    
    return str;
}

// Remove quotes from string value
static void remove_quotes(char *str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// Parse a single line and update config
static int parse_config_line(Config *cfg, char *line) {
    char key[CONFIG_KEY_MAX];
    char value[CONFIG_VALUE_MAX];
    char *eq_pos;
    char *trimmed;
    
    // Skip comments and empty lines
    trimmed = trim_whitespace(line);
    if (trimmed[0] == '#' || trimmed[0] == '\0' || trimmed[0] == '[') {
        return 0;
    }
    
    // Find '=' separator
    eq_pos = strchr(trimmed, '=');
    if (!eq_pos) return 0;
    
    // Extract key
    *eq_pos = '\0';
    strncpy(key, trim_whitespace(trimmed), CONFIG_KEY_MAX - 1);
    key[CONFIG_KEY_MAX - 1] = '\0';
    
    // Extract value
    strncpy(value, trim_whitespace(eq_pos + 1), CONFIG_VALUE_MAX - 1);
    value[CONFIG_VALUE_MAX - 1] = '\0';
    remove_quotes(value);
    
    // Match key and set value
    if (strcmp(key, "i2cBusNumber") == 0) {
        cfg->i2cBusNumber = atoi(value);
    } else if (strcmp(key, "magAddr") == 0) {
        cfg->magAddr = (int)strtol(value, NULL, 0);
    } else if (strcmp(key, "localTempAddr") == 0) {
        cfg->localTempAddr = (int)strtol(value, NULL, 0);
    } else if (strcmp(key, "remoteTempAddr") == 0) {
        cfg->remoteTempAddr = (int)strtol(value, NULL, 0);
    } else if (strcmp(key, "cc_x") == 0) {
        cfg->cc_x = atoi(value);
    } else if (strcmp(key, "cc_y") == 0) {
        cfg->cc_y = atoi(value);
    } else if (strcmp(key, "cc_z") == 0) {
        cfg->cc_z = atoi(value);
    } else if (strcmp(key, "x_gain") == 0) {
        cfg->x_gain = atoi(value);
    } else if (strcmp(key, "y_gain") == 0) {
        cfg->y_gain = atoi(value);
    } else if (strcmp(key, "z_gain") == 0) {
        cfg->z_gain = atoi(value);
    } else if (strcmp(key, "TMRCRate") == 0) {
        cfg->TMRCRate = atoi(value);
    } else if (strcmp(key, "CMMSampleRate") == 0) {
        cfg->CMMSampleRate = atoi(value);
    } else if (strcmp(key, "samplingMode") == 0) {
        cfg->samplingMode = atoi(value);
    } else if (strcmp(key, "NOSRegValue") == 0) {
        cfg->NOSRegValue = atoi(value);
    } else if (strcmp(key, "DRDYdelay") == 0) {
        cfg->DRDYdelay = atoi(value);
    } else if (strcmp(key, "scanI2CBUS") == 0) {
        cfg->scanI2CBUS = atoi(value);
    } else if (strcmp(key, "doBistMask") == 0) {
        cfg->doBistMask = atoi(value);
    } else if (strcmp(key, "readBackCCRegs") == 0) {
        cfg->readBackCCRegs = atoi(value);
    } else if (strcmp(key, "tsMilliseconds") == 0) {
        cfg->tsMilliseconds = atoi(value);
    } else if (strcmp(key, "usePipes") == 0) {
        cfg->usePipes = atoi(value);
    } else if (strcmp(key, "portpath") == 0) {
        strncpy(cfg->portpath, value, CONFIG_VALUE_MAX - 1);
        cfg->portpath[CONFIG_VALUE_MAX - 1] = '\0';
    } else if (strcmp(key, "pipeInPath") == 0) {
        strncpy(cfg->pipeInPath, value, CONFIG_VALUE_MAX - 1);
        cfg->pipeInPath[CONFIG_VALUE_MAX - 1] = '\0';
    } else if (strcmp(key, "pipeOutPath") == 0) {
        strncpy(cfg->pipeOutPath, value, CONFIG_VALUE_MAX - 1);
        cfg->pipeOutPath[CONFIG_VALUE_MAX - 1] = '\0';
    }
    
    return 0;
}

// Read configuration from TOML file
static int config_read(const char *filename, Config *cfg) {
    FILE *fp;
    char line[CONFIG_LINE_MAX];
    
    if (!filename || !cfg) return -1;
    
    fp = fopen(filename, "r");
    if (!fp) {
        if (errno == ENOENT) {
            // File doesn't exist, use defaults
            fprintf(stderr, "Config file '%s' not found, using defaults\n", filename);
            return 0;
        }
        perror("Error opening config file");
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        parse_config_line(cfg, line);
    }
    
    fclose(fp);
    return 0;
}

// Write configuration to TOML file
static int config_write(const char *filename, const Config *cfg) {
    FILE *fp;
    
    if (!filename || !cfg) return -1;
    
    fp = fopen(filename, "w");
    if (!fp) {
        perror("Error creating config file");
        return -1;
    }
    
    fprintf(fp, "# mag-usb Configuration File\n");
    fprintf(fp, "# Generated automatically\n\n");
    
    fprintf(fp, "[i2c]\n");
    fprintf(fp, "i2cBusNumber = %d\n", cfg->i2cBusNumber);
    fprintf(fp, "magAddr = 0x%02X\n", cfg->magAddr);
    fprintf(fp, "localTempAddr = 0x%02X\n", cfg->localTempAddr);
    fprintf(fp, "remoteTempAddr = 0x%02X\n", cfg->remoteTempAddr);
    fprintf(fp, "portpath = \"%s\"\n\n", cfg->portpath);
    
    fprintf(fp, "[magnetometer]\n");
    fprintf(fp, "cc_x = %d\n", cfg->cc_x);
    fprintf(fp, "cc_y = %d\n", cfg->cc_y);
    fprintf(fp, "cc_z = %d\n", cfg->cc_z);
    fprintf(fp, "x_gain = %d\n", cfg->x_gain);
    fprintf(fp, "y_gain = %d\n", cfg->y_gain);
    fprintf(fp, "z_gain = %d\n", cfg->z_gain);
    fprintf(fp, "TMRCRate = %d\n", cfg->TMRCRate);
    fprintf(fp, "CMMSampleRate = %d\n", cfg->CMMSampleRate);
    fprintf(fp, "NOSRegValue = %d\n\n", cfg->NOSRegValue);
    
    fprintf(fp, "[timing]\n");
    fprintf(fp, "samplingMode = %d\n", cfg->samplingMode);
    fprintf(fp, "DRDYdelay = %d\n", cfg->DRDYdelay);
    fprintf(fp, "tsMilliseconds = %d\n\n", cfg->tsMilliseconds);
    
    fprintf(fp, "[options]\n");
    fprintf(fp, "scanI2CBUS = %d\n", cfg->scanI2CBUS);
    fprintf(fp, "doBistMask = %d\n", cfg->doBistMask);
    fprintf(fp, "readBackCCRegs = %d\n", cfg->readBackCCRegs);
    fprintf(fp, "usePipes = %d\n", cfg->usePipes);
    
    if (cfg->pipeInPath[0] != '\0') {
        fprintf(fp, "pipeInPath = \"%s\"\n", cfg->pipeInPath);
    }
    if (cfg->pipeOutPath[0] != '\0') {
        fprintf(fp, "pipeOutPath = \"%s\"\n", cfg->pipeOutPath);
    }
    
    fclose(fp);
    return 0;
}

// Apply configuration to pList structure
static void config_apply_to_plist(const Config *cfg, void *plist_ptr) {
    // Forward declaration - you'll implement this in your main.c
    // to copy values from Config to pList
    extern void apply_config_to_plist(const Config *cfg, void *p);
    apply_config_to_plist(cfg, plist_ptr);
}

#endif // CONFIG_TOML_H
```


## Step 2: Update main.h to include config support

```c++
// ... existing code ...
#include <sys/time.h>
#include "MCP9808.h"
#include "config_toml.h"
//#include "rm3100.h"

// ... existing code ...
```


## Step 3: Add helper functions to your main.c

You'll need to add these functions to your main.c file:

```c++
// Apply configuration from Config struct to pList
void apply_config_to_plist(const Config *cfg, void *plist_ptr) {
    pList *p = (pList *)plist_ptr;
    
    if (!cfg || !p) return;
    
    // Copy values from config to pList
#if(!USE_POLOLU)
    p->i2cBusNumber = cfg->i2cBusNumber;
#endif
    p->magAddr = cfg->magAddr;
    p->localTempAddr = cfg->localTempAddr;
    p->remoteTempAddr = cfg->remoteTempAddr;
    
    p->cc_x = cfg->cc_x;
    p->cc_y = cfg->cc_y;
    p->cc_z = cfg->cc_z;
    p->x_gain = cfg->x_gain;
    p->y_gain = cfg->y_gain;
    p->z_gain = cfg->z_gain;
    
    p->TMRCRate = cfg->TMRCRate;
    p->CMMSampleRate = cfg->CMMSampleRate;
    p->samplingMode = cfg->samplingMode;
    p->NOSRegValue = cfg->NOSRegValue;
    p->DRDYdelay = cfg->DRDYdelay;
    
    p->scanI2CBUS = cfg->scanI2CBUS;
    p->doBistMask = cfg->doBistMask;
    p->readBackCCRegs = cfg->readBackCCRegs;
    p->tsMilliseconds = cfg->tsMilliseconds;
    p->usePipes = cfg->usePipes;
    
    // Only copy paths if not already set
    if (p->portpath == NULL && cfg->portpath[0] != '\0') {
        p->portpath = strdup(cfg->portpath);
    }
    if (p->pipeInPath == NULL && cfg->pipeInPath[0] != '\0') {
        p->pipeInPath = strdup(cfg->pipeInPath);
    }
    if (p->pipeOutPath == NULL && cfg->pipeOutPath[0] != '\0') {
        p->pipeOutPath = strdup(cfg->pipeOutPath);
    }
}

// Load configuration, with command-line overrides
int load_configuration(pList *p, const char *config_file) {
    Config cfg;
    
    // Initialize with defaults
    config_init_defaults(&cfg);
    
    // Read from file (if it exists)
    if (config_read(config_file, &cfg) < 0) {
        fprintf(stderr, "Warning: Could not read config file, using defaults\n");
    }
    
    // Apply to pList (command line args will override later)
    apply_config_to_plist(&cfg, p);
    
    return 0;
}

// Save current configuration
int save_configuration(const pList *p, const char *config_file) {
    Config cfg;
    
    // Initialize to avoid garbage
    config_init_defaults(&cfg);
    
    // Copy current values from pList
#if(!USE_POLOLU)
    cfg.i2cBusNumber = p->i2cBusNumber;
#endif
    cfg.magAddr = p->magAddr;
    cfg.localTempAddr = p->localTempAddr;
    cfg.remoteTempAddr = p->remoteTempAddr;
    
    cfg.cc_x = p->cc_x;
    cfg.cc_y = p->cc_y;
    cfg.cc_z = p->cc_z;
    cfg.x_gain = p->x_gain;
    cfg.y_gain = p->y_gain;
    cfg.z_gain = p->z_gain;
    
    cfg.TMRCRate = p->TMRCRate;
    cfg.CMMSampleRate = p->CMMSampleRate;
    cfg.samplingMode = p->samplingMode;
    cfg.NOSRegValue = p->NOSRegValue;
    cfg.DRDYdelay = p->DRDYdelay;
    
    cfg.scanI2CBUS = p->scanI2CBUS;
    cfg.doBistMask = p->doBistMask;
    cfg.readBackCCRegs = p->readBackCCRegs;
    cfg.tsMilliseconds = p->tsMilliseconds;
    cfg.usePipes = p->usePipes;
    
    if (p->portpath) {
        strncpy(cfg.portpath, p->portpath, CONFIG_VALUE_MAX - 1);
    }
    if (p->pipeInPath) {
        strncpy(cfg.pipeInPath, p->pipeInPath, CONFIG_VALUE_MAX - 1);
    }
    if (p->pipeOutPath) {
        strncpy(cfg.pipeOutPath, p->pipeOutPath, CONFIG_VALUE_MAX - 1);
    }
    
    return config_write(config_file, &cfg);
}
```


## Step 4: Integrate into your main() function

In your `main()` function, call `load_configuration()` before parsing command-line arguments:

```c++
int main(int argc, char** argv) {
    pList params;
    const char *config_file = "config.toml";
    
    // Initialize pList with zeros
    memset(&params, 0, sizeof(pList));
    
    // Load configuration from file (if exists)
    load_configuration(&params, config_file);
    
    // Parse command-line arguments (these will override config file)
    // ... your existing argument parsing code ...
    
    // Rest of your main function
    // ... existing code ...
}
```


## Usage Example

Create a `config.toml` file:

```toml
# mag-usb Configuration File

[i2c]
i2cBusNumber = 1
magAddr = 0x20
localTempAddr = 0x18
remoteTempAddr = 0x19
portpath = "/dev/i2c-1"

[magnetometer]
cc_x = 200
cc_y = 200
cc_z = 200
x_gain = 1
y_gain = 1
z_gain = 1
TMRCRate = 0
CMMSampleRate = 1
NOSRegValue = 0

[timing]
samplingMode = 0
DRDYdelay = 100
tsMilliseconds = 0

[options]
scanI2CBUS = 0
doBistMask = 0
readBackCCRegs = 0
usePipes = 0
```


This solution:
- ✅ Uses no external libraries (fully self-contained)
- ✅ Is header-only with implementation functions
- ✅ Reads TOML configuration on startup
- ✅ Allows command-line arguments to override settings
- ✅ Command-line overrides don't persist between runs
- ✅ Provides a simple API to save configurations when needed