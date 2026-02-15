#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <fat.h>

// Forward declaration of AppData to avoid circular includes
struct AppData; 

// We use void* to avoid needing to include main.h here
void config_load(void* app_ptr);
void config_save(void* app_ptr);

#endif
