//=========================================================================
// config.h
//
// Configuration file handling for mag-usb
// Simple TOML parser for reading config.toml
//
// Author:      David Witten, KD0EAG
// Date:        7/17/25
// License:     GPL 3.0
//=========================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include "main.h"

// Load configuration from TOML file
// Returns 0 on success, -1 if file not found (not an error), -2 on parse error
int load_config(const char *config_path, pList *p);

// Free any heap-allocated string fields inside pList that may have been
// allocated by the configuration loader (via strdup). Safe to call with
// NULL and idempotent for already-freed/NULL fields.
void free_config_strings(pList *p);

#endif // CONFIG_H