#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "heimdall.h"
#include "usb.h"
#include "pit.h"

#define FLASH_CHUNK_SIZE (16 * 1024)

extern PitInfo current_pit; 

// --- Helper Functions ---

const char* heimdall_determine_partition(const char* filename) {
    const char* basename = strrchr(filename, '/');
    basename = (basename) ? basename + 1 : filename;

    char upper_name[64];
    strncpy(upper_name, basename, 63);
    for (int i = 0; upper_name[i]; i++) upper_name[i] = toupper((unsigned char)upper_name[i]);

    char* dot = strchr(upper_name, '.');
    if (dot) *dot = '\0';

    static PitEntry found_entry;
    if (pit_find_partition(&current_pit, upper_name, &found_entry) == 0) {
        return found_entry.partition_name;
    }

    if (strcmp(upper_name, "SYSTEM") == 0) return "FACTORYFS";
    return NULL;
}

// --- Primary Logic ---

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
    long bytes_sent = 0;
    
    while (bytes_sent < total_size) {
        size_t to_read = (total_size - bytes_sent > FLASH_CHUNK_SIZE) ? FLASH_CHUNK_SIZE : (total_size - bytes_sent);
        size_t read = fread(buffer, 1, to_read, f);
        
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
    return usb_init_device();
}

void heimdall_cleanup(void) {
    // Cleanup USB and memory
}

int heimdall_detect_device(void) {
    return usb_init_device();
}

int heimdall_reboot(void) {
    // Send reboot command over USB
    return 0;
}
