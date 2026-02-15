#include "pit.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- Parsing & Validation ---

int pit_parse(const uint8_t* data, uint32_t length, PitInfo* info) {
    if (!data || !info || length < PIT_HEADER_SIZE) return -1;

    // Check Magic (The first 4 bytes of a Samsung PIT)
    uint32_t magic = *(uint32_t*)data;
    if (magic != PIT_MAGIC) {
        return -2; // Invalid Magic
    }

    // Offset 4 is entry count
    info->entry_count = *(uint32_t*)(data + 4);
    
    // Header also contains unknown1 and unknown2 at offsets 8 and 12
    info->unknown1 = *(uint32_t*)(data + 8);
    info->unknown2 = *(uint32_t*)(data + 12);

    // Offset 28 is the device name
    strncpy(info->device_name, (const char*)(data + 28), 63);

    // Parse entries
    uint32_t offset = PIT_HEADER_SIZE;
    for (uint32_t i = 0; i < info->entry_count && i < 64; i++) {
        if (offset + PIT_ENTRY_SIZE > length) break;
        
        memcpy(&info->entries[i], data + offset, PIT_ENTRY_SIZE);
        offset += PIT_ENTRY_SIZE;
    }

    return 0;
}

int pit_validate(const PitInfo* info) {
    if (info->entry_count == 0 || info->entry_count > 64) return -1;
    if (info->device_name[0] == '\0') return -2;
    return 0;
}

// --- Search & Utility ---

int pit_find_partition(const PitInfo* info, const char* name, PitEntry* entry) {
    for (uint32_t i = 0; i < info->entry_count; i++) {
        // Samsung PITs sometimes use partition_name or flash_filename
        if (strcmp(info->entries[i].partition_name, name) == 0 ||
            strcmp(info->entries[i].flash_filename, name) == 0) {
            if (entry) memcpy(entry, &info->entries[i], sizeof(PitEntry));
            return 0;
        }
    }
    return -1; // Not found
}

uint32_t pit_calculate_size(const PitInfo* info) {
    return PIT_HEADER_SIZE + (info->entry_count * PIT_ENTRY_SIZE);
}

// --- Writing & Serialization ---

int pit_write(const PitInfo* info, uint8_t** data, uint32_t* length) {
    if (!info || !data || !length) return -1;

    *length = pit_calculate_size(info);
    *data = (uint8_t*)malloc(*length);
    if (!*data) return -1;

    uint8_t* ptr = *data;
    memset(ptr, 0, *length);

    // Write Header
    *(uint32_t*)ptr = PIT_MAGIC;
    *(uint32_t*)(ptr + 4) = info->entry_count;
    *(uint32_t*)(ptr + 8) = info->unknown1;
    *(uint32_t*)(ptr + 12) = info->unknown2;
    memcpy(ptr + 28, info->device_name, 32);

    // Write Entries
    uint32_t offset = PIT_HEADER_SIZE;
    for (uint32_t i = 0; i < info->entry_count; i++) {
        memcpy(ptr + offset, &info->entries[i], PIT_ENTRY_SIZE);
        offset += PIT_ENTRY_SIZE;
    }

    return 0;
}

// --- Debugging ---

int pit_print(const PitInfo* info) {
    if (!info) return -1;
    printf("PIT Device: %s\n", info->device_name);
    printf("Partitions: %u\n", info->entry_count);
    printf("-------------------------------\n");
    for (uint32_t i = 0; i < info->entry_count; i++) {
        printf("[%02d] %-16s | Block: %u | Size: %u\n", 
               i, info->entries[i].partition_name, 
               info->entries[i].block_size, info->entries[i].block_count);
    }
    return 0;
}
