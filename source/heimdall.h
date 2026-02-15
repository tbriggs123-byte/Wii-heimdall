// source/heimdall.h
#ifndef HEIMDALL_H
#define HEIMDALL_H

#include <gccore.h>
#include <stdint.h>

// Callback types
typedef int (*ProgressCallback)(float progress, const char* status);
typedef void (*LogCallback)(const char* message, int type);

// PIT structures
typedef struct {
    uint32_t binary_type;
    uint32_t device_type;
    uint32_t identifier;
    uint32_t attributes;
    uint32_t update_attributes;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t file_offset;
    uint32_t file_size;
    char partition_name[32];
    char flash_filename[32];
    char fota_filename[32];
} PitEntry;

typedef struct {
    uint32_t entry_count;
    uint32_t unknown1;
    uint32_t unknown2;
    char device_name[64];
    PitEntry entries[64];
} PitInfo;

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