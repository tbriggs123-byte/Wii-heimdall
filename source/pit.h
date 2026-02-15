// source/pit.h
#ifndef PIT_H
#define PIT_H

#include <stdint.h>

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

// PIT functions
int pit_parse(const uint8_t* data, uint32_t length, PitInfo* info);
int pit_write(const PitInfo* info, uint8_t** data, uint32_t* length);
int pit_print(const PitInfo* info);
int pit_find_partition(const PitInfo* info, const char* name, PitEntry* entry);
uint32_t pit_calculate_size(const PitInfo* info);
int pit_validate(const PitInfo* info);

// Samsung PIT constants
#define PIT_MAGIC 0x12349876
#define PIT_HEADER_SIZE 44
#define PIT_ENTRY_SIZE 132

#endif