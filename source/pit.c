// source/pit.c
#include "pit.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Parse PIT file
int pit_parse(const uint8_t* data, uint32_t length, PitInfo* info) {
    if (!data || !info || length < PIT_HEADER_SIZE) {
        return -1;
    }
    
    memset(info, 0, sizeof(PitInfo));
    
    // Parse header
    info->entry_count = *(uint32_t*)(data + 0);
    info->unknown1 = *(uint32_t*)(data + 4);
    info->unknown2 = *(uint32_t*)(data + 8);
    
    // Copy device name
    strncpy(info->device_name, (const char*)(data + 12), 
            sizeof(info->device_name) - 1);
    
    // Validate entry count
    uint32_t expected_size = PIT_HEADER_SIZE + (info->entry_count * PIT_ENTRY_SIZE);
    if (length < expected_size) {
        return -1;
    }
    
    // Parse entries
    const uint8_t* entry_ptr = data + PIT_HEADER_SIZE;
    
    for (uint32_t i = 0; i < info->entry_count && i < 64; i++) {
        PitEntry* entry = &info->entries[i];
        
        entry->binary_type = *(uint32_t*)(entry_ptr + 0);
        entry->device_type = *(uint32_t*)(entry_ptr + 4);
        entry->identifier = *(uint32_t*)(entry_ptr + 8);
        entry->attributes = *(uint32_t*)(entry_ptr + 12);
        entry->update_attributes = *(uint32_t*)(entry_ptr + 16);
        entry->block_size = *(uint32_t*)(entry_ptr + 20);
        entry->block_count = *(uint32_t*)(entry_ptr + 24);
        entry->file_offset = *(uint32_t*)(entry_ptr + 28);
        entry->file_size = *(uint32_t*)(entry_ptr + 32);
        
        // Copy strings (null-terminated in PIT)
        strncpy(entry->partition_name, (const char*)(entry_ptr + 36), 31);
        strncpy(entry->flash_filename, (const char*)(entry_ptr + 68), 31);
        strncpy(entry->fota_filename, (const char*)(entry_ptr + 100), 31);
        
        // Ensure null termination
        entry->partition_name[31] = '\0';
        entry->flash_filename[31] = '\0';
        entry->fota_filename[31] = '\0';
        
        entry_ptr += PIT_ENTRY_SIZE;
    }
    
    return 0;
}

// Write PIT structure to binary
int pit_write(const PitInfo* info, uint8_t** data, uint32_t* length) {
    if (!info || !data || !length) {
        return -1;
    }
    
    // Calculate size
    uint32_t total_size = PIT_HEADER_SIZE + (info->entry_count * PIT_ENTRY_SIZE);
    
    // Allocate buffer
    *data = malloc(total_size);
    if (!*data) {
        return -1;
    }
    
    memset(*data, 0, total_size);
    
    // Write header
    uint8_t* ptr = *data;
    
    *(uint32_t*)(ptr + 0) = info->entry_count;
    *(uint32_t*)(ptr + 4) = info->unknown1;
    *(uint32_t*)(ptr + 8) = info->unknown2;
    
    // Write device name
    strncpy((char*)(ptr + 12), info->device_name, 32);
    
    // Write entries
    ptr += PIT_HEADER_SIZE;
    
    for (uint32_t i = 0; i < info->entry_count; i++) {
        const PitEntry* entry = &info->entries[i];
        
        *(uint32_t*)(ptr + 0) = entry->binary_type;
        *(uint32_t*)(ptr + 4) = entry->device_type;
        *(uint32_t*)(ptr + 8) = entry->identifier;
        *(uint32_t*)(ptr + 12) = entry->attributes;
        *(uint32_t*)(ptr + 16) = entry->update_attributes;
        *(uint32_t*)(ptr + 20) = entry->block_size;
        *(uint32_t*)(ptr + 24) = entry->block_count;
        *(uint32_t*)(ptr + 28) = entry->file_offset;
        *(uint32_t*)(ptr + 32) = entry->file_size;
        
        // Write strings
        strncpy((char*)(ptr + 36), entry->partition_name, 32);
        strncpy((char*)(ptr + 68), entry->flash_filename, 32);
        strncpy((char*)(ptr + 100), entry->fota_filename, 32);
        
        ptr += PIT_ENTRY_SIZE;
    }
    
    *length = total_size;
    return 0;
}

// Print PIT information
int pit_print(const PitInfo* info) {
    if (!info) {
        return -1;
    }
    
    printf("=== PIT Information ===\n");
    printf("Device: %s\n", info->device_name);
    printf("Entries: %u\n", info->entry_count);
    printf("Unknown1: 0x%08X\n", info->unknown1);
    printf("Unknown2: 0x%08X\n", info->unknown2);
    printf("\n");
    
    printf("Partitions:\n");
    printf("ID  Name                  Size      Blocks  File\n");
    printf("--- -------------------- --------- ------- --------------------\n");
    
    for (uint32_t i = 0; i < info->entry_count; i++) {
        const PitEntry* entry = &info->entries[i];
        
        uint32_t size_mb = (entry->block_count * entry->block_size) / (1024 * 1024);
        
        printf("%3u %-20s %4u MB %7u %s\n",
               entry->identifier,
               entry->partition_name,
               size_mb,
               entry->block_count,
               entry->flash_filename[0] ? entry->flash_filename : "(none)");
    }
    
    printf("=======================\n");
    
    return 0;
}

// Find partition by name
int pit_find_partition(const PitInfo* info, const char* name, PitEntry* entry) {
    if (!info || !name || !entry) {
        return -1;
    }
    
    for (uint32_t i = 0; i < info->entry_count; i++) {
        if (strcasecmp(info->entries[i].partition_name, name) == 0) {
            *entry = info->entries[i];
            return 0;
        }
    }
    
    return -1;
}

// Calculate total size from PIT
uint32_t pit_calculate_size(const PitInfo* info) {
    if (!info) return 0;
    
    uint32_t total_blocks = 0;
    
    for (uint32_t i = 0; i < info->entry_count; i++) {
        total_blocks += info->entries[i].block_count;
    }
    
    // Assume standard block size if not set
    uint32_t block_size = 512; // Default
    if (info->entry_count > 0) {
        block_size = info->entries[0].block_size;
    }
    
    return total_blocks * block_size;
}

// Validate PIT structure
int pit_validate(const PitInfo* info) {
    if (!info) {
        return -1;
    }
    
    if (info->entry_count > 64) {
        return -1; // Too many entries
    }
    
    // Check for duplicate identifiers
    for (uint32_t i = 0; i < info->entry_count; i++) {
        for (uint32_t j = i + 1; j < info->entry_count; j++) {
            if (info->entries[i].identifier == info->entries[j].identifier) {
                return -1; // Duplicate ID
            }
        }
    }
    
    return 0;
}