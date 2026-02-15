// source/heimdall.h
#ifndef HEIMDALL_H
#define HEIMDALL_H

#include <gccore.h>
#include <stdint.h>
#include "pit.h"
// Callback types
typedef int (*ProgressCallback)(float progress, const char* status);
typedef void (*LogCallback)(const char* message, int type);

// Heimdall functions
int heimdall_init(void);
void heimdall_cleanup(void);
int heimdall_detect_device(void);
int heimdall_load_pit(const char* filename);
PitInfo* heimdall_get_pit_info(void);
const char* heimdall_determine_partition(const char* filename);
int heimdall_flash_file(const char* filename, const char* partition, 
                       ProgressCallback callback);
int heimdall_reboot(void);
int heimdall_download_pit(void);
int heimdall_print_pit(void);

// Callback setters
void heimdall_set_progress_callback(ProgressCallback cb);
void heimdall_set_log_callback(LogCallback cb);

// Utility functions
int heimdall_verify_file(const char* filename);
uint32_t heimdall_calculate_checksum(const uint8_t* data, uint32_t length);
int heimdall_is_samsung_device(uint16_t vid, uint16_t pid);

#endif
