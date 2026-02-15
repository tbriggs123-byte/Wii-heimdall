#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "heimdall.h"
#include "usb.h"  // <--- This must be here!
#include "pit.h"

#define FLASH_CHUNK_SIZE (16 * 1024)

extern PitInfo current_pit; 

int heimdall_flash_file(const char* filename, const char* partition, int (*progress_cb)(float, const char*)) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long total_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // This now matches the name in usb.h
    if (usb_start_flash_session(partition) != 0) {
        fclose(f);
        return -2;
    }

    uint8_t* buffer = malloc(FLASH_CHUNK_SIZE);
    long bytes_sent = 0;
    
    while (bytes_sent < total_size) {
        size_t to_read = (total_size - bytes_sent > FLASH_CHUNK_SIZE) ? FLASH_CHUNK_SIZE : (total_size - bytes_sent);
        size_t read = fread(buffer, 1, to_read, f);
        
        // This now matches the name in usb.h
        if (usb_send_data(buffer, read) != 0) break;

        bytes_sent += read;
        if (progress_cb) {
            float percent = (float)bytes_sent / (float)total_size;
            progress_cb(percent, "Transferring...");
        }
    }

    usb_end_flash_session();
    free(buffer);
    fclose(f);
    return 0;
}

int heimdall_init(void) {
    return usb_init_device(); // Matches the new function in usb.c
}

// ... rest of your heimdall.c functions ...
// Add these to source/heimdall.c

// 1. Detect Device
int heimdall_detect_device(void) {
    if (usb_init_device() == 0) {
        if (usb_is_connected()) return 0;
    }
    return -1;
}

// 2. Load PIT from file
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

    // Use the pit_parse function from pit.c
    int result = pit_parse(buffer, size, &current_pit);
    free(buffer);
    return result;
}

// 3. Get PIT Info
PitInfo* heimdall_get_pit_info(void) {
    return &current_pit;
}
