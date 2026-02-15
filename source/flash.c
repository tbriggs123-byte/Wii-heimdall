// source/flash.c
#include "flash.h"
#include "usb.h"
#include "fileio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Flash state
static int flash_busy = 0;
static float flash_progress = 0.0f;
static char flash_status[128] = "";
static FlashProgressCallback progress_cb = NULL;

// Initialize flash subsystem
int flash_init(void) {
    flash_busy = 0;
    flash_progress = 0.0f;
    flash_status[0] = '\0';
    return 0;
}

// Cleanup flash subsystem
void flash_cleanup(void) {
    if (flash_busy) {
        flash_abort();
    }
}

// Flash file to partition
int flash_file(const char* filename, const char* partition, 
               FlashProgressCallback callback) {
    if (flash_busy) {
        return -1; // Already busy
    }
    
    if (!filename || !partition) {
        return -1;
    }
    
    // Set state
    flash_busy = 1;
    flash_progress = 0.0f;
    strcpy(flash_status, "Starting flash");
    progress_cb = callback;
    
    // Report start
    if (progress_cb) {
        progress_cb(0.0f, "Opening file");
    }
    
    // Read file
    uint32_t file_size;
    uint8_t* file_data = fileio_read_file(filename, &file_size);
    if (!file_data) {
        flash_busy = 0;
        return -1;
    }
    
    // Flash the data
    int result = flash_data(file_data, file_size, partition, callback);
    
    // Cleanup
    free(file_data);
    flash_busy = 0;
    
    return result;
}

// Flash raw data to partition
int flash_data(const uint8_t* data, uint32_t length, const char* partition,
               FlashProgressCallback callback) {
    if (!data || length == 0 || !partition) {
        return -1;
    }
    
    // Update status
    strcpy(flash_status, "Preparing flash");
    flash_progress = 0.0f;
    
    if (callback) {
        callback(0.0f, "Initializing");
    }
    
    // Determine file type from partition name
    uint32_t file_type = 0;
    if (strcasecmp(partition, "BOOT") == 0) {
        file_type = 0x00; // Bootloader
    } else if (strcasecmp(partition, "RECOVERY") == 0) {
        file_type = 0x03; // Recovery
    } else if (strcasecmp(partition, "SYSTEM") == 0) {
        file_type = 0x02; // System
    } else if (strcasecmp(partition, "CACHE") == 0) {
        file_type = 0x04; // Cache
    } else if (strcasecmp(partition, "MODEM") == 0) {
        file_type = 0x01; // Modem
    } else {
        file_type = 0x05; // Other
    }
    
    // Send file header
    if (callback) {
        callback(0.1f, "Sending header");
    }
    
    char temp_filename[32];
    snprintf(temp_filename, sizeof(temp_filename), "%s.img", partition);
    
    if (samsung_send_file_header(temp_filename, length, file_type) != 0) {
        strcpy(flash_status, "Header failed");
        return -1;
    }
    
    // Send file in chunks
    const uint32_t CHUNK_SIZE = 0x40000; // 256KB
    uint32_t sequence = 0;
    uint32_t sent = 0;
    
    while (sent < length) {
        uint32_t chunk_size = length - sent;
        if (chunk_size > CHUNK_SIZE) {
            chunk_size = CHUNK_SIZE;
        }
        
        // Update progress
        flash_progress = (float)sent / length;
        char status[64];
        snprintf(status, sizeof(status), "Sending chunk %u", sequence + 1);
        strcpy(flash_status, status);
        
        if (callback) {
            callback(flash_progress, status);
        }
        
        // Send chunk
        if (samsung_send_file_part(data + sent, chunk_size, sequence) != 0) {
            strcpy(flash_status, "Chunk failed");
            return -1;
        }
        
        // Wait for ACK
        if (samsung_wait_ack() != 0) {
            strcpy(flash_status, "No ACK received");
            return -1;
        }
        
        sent += chunk_size;
        sequence++;
    }
    
    // Send file end
    if (callback) {
        callback(0.9f, "Finalizing");
    }
    
    uint32_t checksum = 0; // Would calculate actual checksum
    if (samsung_send_file_end(length, checksum) != 0) {
        strcpy(flash_status, "End failed");
        return -1;
    }
    
    // Complete
    flash_progress = 1.0f;
    strcpy(flash_status, "Flash complete");
    
    if (callback) {
        callback(1.0f, "Complete");
    }
    
    return 0;
}

// Verify flash
int flash_verify(const char* filename, const char* partition) {
    // Not implemented in this version
    return 0;
}

// Abort current flash
int flash_abort(void) {
    if (!flash_busy) {
        return 0;
    }
    
    // Send abort command to device
    uint8_t abort_cmd[] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00};
    usb_send_bulk(abort_cmd, sizeof(abort_cmd));
    
    flash_busy = 0;
    flash_progress = 0.0f;
    strcpy(flash_status, "Aborted");
    
    return 0;
}

// Check if flash is busy
int flash_is_busy(void) {
    return flash_busy;
}

// Get current progress
float flash_get_progress(void) {
    return flash_progress;
}

// Get current status
const char* flash_get_status(void) {
    return flash_status;
}

// Samsung protocol implementation

// Send file header
int samsung_send_file_header(const char* filename, uint32_t file_size, 
                             uint32_t file_type) {
    uint8_t header[1024];
    memset(header, 0, sizeof(header));
    
    // Header structure
    *(uint32_t*)(header + 0) = 0x00000000; // Magic
    *(uint32_t*)(header + 4) = file_size;
    *(uint32_t*)(header + 8) = file_type;
    *(uint32_t*)(header + 12) = 0x00000000; // Unknown
    
    // Filename
    strncpy((char*)(header + 16), filename, 256);
    
    return usb_send_bulk(header, 1024);
}

// Send file part
int samsung_send_file_part(const uint8_t* data, uint32_t length, 
                           uint32_t sequence) {
    // Create part header
    uint8_t header[16];
    memset(header, 0, sizeof(header));
    
    *(uint32_t*)(header + 0) = 0x00000001; // Part magic
    *(uint32_t*)(header + 4) = sequence;
    *(uint32_t*)(header + 8) = length;
    *(uint32_t*)(header + 12) = 0x00000000; // Reserved
    
    // Send header
    if (usb_send_bulk(header, sizeof(header)) != sizeof(header)) {
        return -1;
    }
    
    // Send data
    if (usb_send_bulk(data, length) != length) {
        return -1;
    }
    
    return 0;
}

// Send file end
int samsung_send_file_end(uint32_t file_size, uint32_t checksum) {
    uint8_t end[16];
    memset(end, 0, sizeof(end));
    
    *(uint32_t*)(end + 0) = 0x00000002; // End magic
    *(uint32_t*)(end + 4) = file_size;
    *(uint32_t*)(end + 8) = checksum;
    *(uint32_t*)(end + 12) = 0x00000000; // Reserved
    
    return usb_send_bulk(end, sizeof(end));
}

// Wait for ACK
int samsung_wait_ack(void) {
    uint8_t ack_buffer[16];
    uint8_t* p_ack = ack_buffer;  // Create a pointer to the buffer
    uint32_t len = 16;            // Create a variable for the length
    if (usb_receive_bulk(&p_ack, &len) != 0) {
        return -1;
    }
    
    // Check if it's an ACK
    if (*(uint32_t*)ack_buffer != 0x00000000) {
        return -1;
    }
    
    return 0;
}

// Send PIT file
int samsung_send_pit(const uint8_t* pit_data, uint32_t pit_size) {
    // Send PIT command
    uint8_t pit_cmd[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    if (usb_send_bulk(pit_cmd, sizeof(pit_cmd)) != sizeof(pit_cmd)) {
        return -1;
    }
    
    // Send PIT data
    if (usb_send_bulk(pit_data, pit_size) != pit_size) {
        return -1;
    }
    
    // Wait for ACK
    return samsung_wait_ack();
}
