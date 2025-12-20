//=========================================================================
// config.c
//
// Simple TOML configuration parser implementation
//=========================================================================
#include "config.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define MAX_LINE_LENGTH 512
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 256

//---------------------------------------------------------------
// Helper: Trim leading and trailing whitespace
//---------------------------------------------------------------
static char *trim_whitespace(char *str)
{
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str))
    {
        str++;
    }
    if(*str == 0)
    {
        return str;
    }
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end))
    {
        end--;
    }
    end[1] = '\0';
    return str;
}

//---------------------------------------------------------------
// Helper: Remove quotes from string value
//---------------------------------------------------------------
static char *remove_quotes(char *str)
{
    size_t len = strlen(str);
    if(len >= 2 && str[0] == '"' && str[len-1] == '"')
    {
        str[len-1] = '\0';
        return str + 1;
    }
    return str;
}

//---------------------------------------------------------------
// Helper: Parse boolean value (true/false)
//---------------------------------------------------------------
static int parse_bool(const char *value)
{
    if(strcmp(value, "true") == 0) return TRUE;
    if(strcmp(value, "false") == 0) return FALSE;
    return atoi(value) != 0;
}

//---------------------------------------------------------------
// Helper: Parse integer with support for hex (0x) format
//---------------------------------------------------------------
static int parse_int(const char *value)
{
    return (int)strtol(value, NULL, 0);
}

//---------------------------------------------------------------
// Helper: Parse double/float value
//---------------------------------------------------------------
static double parse_double(const char *value)
{
    return strtod(value, NULL);
}

//---------------------------------------------------------------
// Helper: Check if line is a section header [section]
//---------------------------------------------------------------
static int is_section_header(const char *line, char *section_name)
{
    const char *ptr = line;

    // Skip whitespace
    while(isspace((unsigned char)*ptr))
    {
        ptr++;
    }
    if(*ptr != '[')
    {
        return 0;
    }
    ptr++;

    // Extract section name
    int i = 0;
    while(*ptr && *ptr != ']' && i < MAX_KEY_LENGTH - 1)
    {
        section_name[i++] = *ptr++;
    }
    section_name[i] = '\0';

    // Trim section name
    char *trimmed = trim_whitespace(section_name);
    if(trimmed != section_name)
    {
        memmove(section_name, trimmed, strlen(trimmed) + 1);
    }
    return (*ptr == ']');
}

//---------------------------------------------------------------
// Helper: Parse key = value line
//---------------------------------------------------------------
static int parse_key_value(const char *line, char *key, char *value)
{
    const char *equals = strchr(line, '=');
    if(!equals)
    {
        return 0;
    }
    // Extract key
    size_t key_len = equals - line;
    if(key_len >= MAX_KEY_LENGTH)
    {
        key_len = MAX_KEY_LENGTH - 1;
    }
    memcpy(key, line, key_len);
    key[key_len] = '\0';

    // Extract value
    snprintf(value, MAX_VALUE_LENGTH, "%s", equals + 1);

    // Trim both
    char *trimmed_key = trim_whitespace(key);
    char *trimmed_value = trim_whitespace(value);
    if(trimmed_key != key)
    {
        memmove(key, trimmed_key, strlen(trimmed_key) + 1);
    }
    if(trimmed_value != value)
    {
        memmove(value, trimmed_value, strlen(trimmed_value) + 1);
    }
    // Remove quotes from value
    trimmed_value = remove_quotes(value);
    if(trimmed_value != value)
    {
        memmove(value, trimmed_value, strlen(trimmed_value) + 1);
    }
    return 1;
}

//---------------------------------------------------------------
// Process configuration value based on section and key
//---------------------------------------------------------------
static void process_config_value(pList *p, const char *section, const char *key, const char *value)
{
    // [node_information] section
    if(strcmp(section, "node_information") == 0)
    {
        if(strcmp(key, "maintainer") == 0)
        {
            p->maintainer = strdup(value);
        }
        else if(strcmp(key, "maintainer_email") == 0)
        {
            p->maintainer_email = strdup(value);
        }
    }
    // [node_location] section
    else if(strcmp(section, "node_location") == 0)
    {
        if(strcmp(key, "latitude") == 0)
        {
            p->latitude = strdup(value);
        }
        else if(strcmp(key, "longitude") == 0)
        {
            p->longitude = strdup(value);
        }
        else if(strcmp(key, "elevation") == 0)
        {
            p->elevation = strdup(value);
        }
        else if(strcmp(key, "grid_square") == 0)
        {
            p->grid_square = strdup(value);
        }
    }
    // [i2c] section
    else if(strcmp(section, "i2c") == 0)
    {
        if(strcmp(key, "portpath") == 0)
        {
            if(p->portpath != NULL)
            {
                snprintf(p->portpath, PATH_MAX, "%s", value);
            }
            else
            {
                fprintf(OUTPUT_ERROR, "ERROR: p->portpath is NULL!\n");
            }
        }
        else if(strcmp(key, "bus_number") == 0)
        {
            p->i2cBusNumber = parse_int(value);
        }
        else if(strcmp(key, "scan_bus") == 0)
        {
            p->scanI2CBUS = parse_bool(value);
        }
        else if(strcmp(key, "use_I2C_converter") == 0)
        {
            p->use_I2C_converter = parse_bool(value);
        }
    }
    // [magnetometer] section
    else if(strcmp(section, "magnetometer") == 0)
    {
        if(strcmp(key, "address") == 0)
        {
            p->magAddr = parse_int(value);
        }
        else if(strcmp(key, "cc_x") == 0)
        {
            p->cc_x = parse_int(value);
        }
        else if(strcmp(key, "cc_y") == 0)
        {
            p->cc_y = parse_int(value);
        }
        else if(strcmp(key, "cc_z") == 0)
        {
            p->cc_z = parse_int(value);
        }
        else if(strcmp(key, "gain_x") == 0)
        {
            p->x_gain = parse_double(value);
        }
        else if(strcmp(key, "gain_y") == 0)
        {
            p->y_gain = parse_double(value);
        }
        else if(strcmp(key, "gain_z") == 0)
        {
            p->z_gain = parse_double(value);
        }
        else if(strcmp(key, "tmrc_rate") == 0)
        {
            p->TMRCRate = parse_int(value);
        }
        else if(strcmp(key, "nos_reg_value") == 0)
        {
            p->NOSRegValue = parse_int(value);
        }
        else if(strcmp(key, "drdy_delay") == 0)
        {
            p->DRDYdelay = parse_int(value);
        }
        else if(strcmp(key, "sampling_mode") == 0)
        {
            if(strcmp(value, "POLL") == 0)
            {
                p->samplingMode = POLL;
            }
            else if(strcmp(value, "CMM") == 0)
            {
                p->samplingMode = CMM;
            }
        }
        else if(strcmp(key, "cmm_sample_rate") == 0)
        {
            p->CMMSampleRate = parse_int(value);
        }
        else if(strcmp(key, "readback_cc_regs") == 0)
        {
            p->readBackCCRegs = parse_bool(value);
        }
    }
    // [mag_orientation] section
    else if(strcmp(section, "mag_orientation") == 0)
    {
        // Only allow -180, -90, 0, 90, 180. Default to 0 if invalid.
        int v = parse_int(value);
        if(strcmp(key, "mag_translate_x") == 0)
        {
            p->mag_translate_x = (v == -180 || v == -90 || v == 0 || v == 90 || v == 180) ? v : 0;
        }
        else if(strcmp(key, "mag_translate_y") == 0)
        {
            p->mag_translate_y = (v == -180 || v == -90 || v == 0 || v == 90 || v == 180) ? v : 0;
        }
        else if(strcmp(key, "mag_translate_z") == 0)
        {
            p->mag_translate_z = (v == -180 || v == -90 || v == 0 || v == 90 || v == 180) ? v : 0;
        }
    }
    // [temperature] section
    else if(strcmp(section, "temperature") == 0)
    {
        if(strcmp(key, "remote_temp_address") == 0)
        {
            p->remoteTempAddr = parse_int(value);
        }
    }
    // [output] section
    else if(strcmp(section, "output") == 0)
    {
        if(strcmp(key, "write_logs") == 0)
        {
            p->write_logs = parse_bool(value);
        }
        else if(strcmp(key, "log_output_path") == 0)
        {
            p->log_output_path = strdup(value);
        }
        else if(strcmp(key, "create_log_path_if_empty") == 0)
        {
            p->create_log_path_if_empty = parse_bool(value);
        }
        else if(strcmp(key, "use_pipes") == 0)
        {
            p->usePipes = parse_bool(value);
        }
        else if(strcmp(key, "pipe_in_path") == 0)
        {
            p->pipeInPath = strdup(value);
        }
        else if(strcmp(key, "pipe_out_path") == 0)
        {
            p->pipeOutPath = strdup(value);
        }
    }
}

//---------------------------------------------------------------
// Main configuration loader
//---------------------------------------------------------------
int load_config(const char *config_path, pList *p)
{
    FILE *fp = fopen(config_path, "r");
    if(!fp)
    {
        if(errno == ENOENT)
        {
            // File doesn't exist - not an error, will use defaults
            fprintf(stderr, "File doesn't exist:  '%s'. Will use defaults.\n", config_path);
            return -1;
        }
        else
        {
            fprintf(stderr, "Error opening config file '%s': %s\n", config_path, strerror(errno));
            return -1;
        }
    }
    else
    {
        fprintf(stderr, "Using configuration file: '%s'.\n", config_path);
    }

    char line[MAX_LINE_LENGTH];
    char current_section[MAX_KEY_LENGTH] = "";
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int line_num = 0;
    int parse_errors = 0;

    while(fgets(line, sizeof(line), fp))
    {
        line_num++;

        // Remove newline and carriage return
        size_t len = strlen(line);
        if(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
        {
            line[len-1] = '\0';
            len--;
        }
        if(len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'))
        {
            line[len-1] = '\0';
        }

//        fprintf(stderr, "Line: %i, %i, %s\n", line_num, (int) len, line);
//        fflush(stderr);

        char *trimmed = trim_whitespace(line);

        // Skip empty lines and comments
        if(trimmed[0] == '\0' || trimmed[0] == '#')
        {
            continue;
        }

        // Check for section header
        if(is_section_header(trimmed, current_section))
        {
#if(__DEBUG)
//            fprintf(OUTPUT_PRINT, "Config: [%s]\n", current_section);
#endif
            continue;
        }

        // Parse key-value pair
        if(parse_key_value(trimmed, key, value))
        {
#if(__DEBUG)
//            fprintf(OUTPUT_PRINT, "Config:      %s.%s = %s\n", current_section, key, value);
#endif
            process_config_value(p, current_section, key, value);
        }
        else
        {
            fprintf(stderr, "Warning: Could not parse line %d in %s: %s\n", line_num, config_path, line);
            parse_errors++;
        }
    }
    fclose(fp);
    if(parse_errors > 0)
    {
        fprintf(stderr, "Warning: %d parse error(s) in config file\n", parse_errors);
        return -2;
    }
    // Successfully loaded configuration
#if(__DEBUG)
//    fprintf(OUTPUT_PRINT, "Successfully loaded configuration\n");
#endif
    return 0;
}

//===============================================================
// Cleanup: free any heap-allocated string fields inside pList
//===============================================================
void free_config_strings(pList *p)
{
    if(p == NULL)
    {
        return;
    }
    // Only free strings we allocated via strdup in the config loader.
    // Do NOT free: p->portpath (points to static buffer owned by main.c)
    // Do NOT free: p->Version   (points to static static array)

    if(p->maintainer)
    {
        free(p->maintainer);
        p->maintainer = NULL;
    }
    if(p->maintainer_email)
    {
        free(p->maintainer_email);
        p->maintainer_email = NULL;
    }
    if(p->latitude)
    {
        free(p->latitude);
        p->latitude = NULL;
    }
    if(p->longitude)
    {
        free(p->longitude);
        p->longitude = NULL;
    }
    if(p->elevation)
    {
        free(p->elevation);
        p->elevation = NULL;
    }
    if(p->grid_square)
    {
        free(p->grid_square);
        p->grid_square = NULL;
    }
    if(p->log_output_path)
    {
        free(p->log_output_path);
        p->log_output_path = NULL;
    }
    if(p->pipeInPath)
    {
        free(p->pipeInPath);
        p->pipeInPath = NULL;
    }
    if(p->pipeOutPath)
    {
        free(p->pipeOutPath);
        p->pipeOutPath = NULL;
    }
}
