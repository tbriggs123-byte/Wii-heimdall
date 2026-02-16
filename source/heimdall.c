#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "heimdall.h"
#include "usb.h"
#include "pit.h"

#define FLASH_CHUNK_SIZE (16 * 1024)

// FIX 1: Remove 'extern'. This file MUST define the variable 
// so the linker has a physical memory address for it.
PitInfo current_pit; 

// --- Core Heimdall Logic ---

int heimdall_init(void) {
    return usb_init_device();
}

void heimdall_cleanup(void) {
    usb_cleanup();
}

int heimdall_detect_device(void) {
    if (usb_init_device() == 0) {
        if (usb_is_connected()) return 0;
    }
    return -1;
}

int heimdall_reboot(void) {
    // Samsung Download Mode usually responds to "REBT" or "REST"
    return usb_send_samsung_cmd("REBT", 0); 
}

// --- Partition Management ---

const char* heimdall_determine_partition(const char* filename) {
    if (!filename) return NULL;
    
    // Simple mapping based on filename
    if (strstr(filename, "recovery.img")) return "RECOVERY";
    if (strstr(filename, "system.img"))   return "SYSTEM";
    if (strstr(filename, "boot.img"))     return "BOOT";
    if (strstr(filename, "cache.img"))    return "CACHE";
    if (strstr(filename, "modem.bin"))    return "MODEM";
    if (strstr(filename, "zImage"))       return "KERNEL";
    
    return NULL;
}

int heimdall_load_pit(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return -2;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    int result = pit_parse(buffer, size, &current_pit);
    free(buffer);
    return result;
}

PitInfo* heimdall_get_pit_info(void) {
    return &current_pit;
}

// --- Flashing Logic ---

int heimdall_flash_file(const char* filename, const char* partition, int (*progress_cb)(float, const char*)) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long total_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (usb_start_flash_session(partition) != 0) {
        fclose(f);
        return -2;
    }

    uint8_t* buffer = malloc(FLASH_CHUNK_SIZE);
    if (!buffer) {
        fclose(f);
        return -3;
    }

    long bytes_sent = 0;
    int status = 0;
    
    while (bytes_sent < total_size) {
        size_t to_read = (total_size - bytes_sent > FLASH_CHUNK_SIZE) ? FLASH_CHUNK_SIZE : (total_size - bytes_sent);
        size_t read_bytes = fread(buffer, 1, to_read, f);
        
        if (usb_send_data(buffer, (uint32_t)read_bytes) != 0) {
            status = -4;
            break;
        }

        bytes_sent += read_bytes;
        if (progress_cb) {
            float percent = (float)bytes_sent / (float)total_size;
            progress_cb(percent, "Transferring...");
        }
    }

    usb_end_flash_session();
    free(buffer);
    fclose(f);
    return status;
}
