// source/heimdall.c
#include "heimdall.h"
#include "usb.h"
#include "pit.h"
#include "flash.h"
#include "fileio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "heimdall.h"
#include "pit.h"

// Reference to the PIT loaded in pit.c or heimdall_load_pit
extern PitInfo current_pit; 

const char* heimdall_determine_partition(const char* filename) {
    // 1. Strip path (e.g., "sd:/recovery.img" -> "recovery.img")
    const char* basename = strrchr(filename, '/');
    if (basename) basename++;
    else basename = filename;

    // 2. Create a uppercase version for comparison
    char upper_name[64];
    strncpy(upper_name, basename, 63);
    for (int i = 0; upper_name[i]; i++) {
        upper_name[i] = toupper((unsigned char)upper_name[i]);
    }

    // 3. Remove extension (e.g., "RECOVERY.IMG" -> "RECOVERY")
    char* dot = strchr(upper_name, '.');
    if (dot) *dot = '\0';

    // 4. Try to find an exact match in the PIT
    static PitEntry found_entry;
    if (pit_find_partition(&current_pit, upper_name, &found_entry) == 0) {
        return found_entry.partition_name;
    }

    // 5. Common aliases / Fallbacks
    if (strcmp(upper_name, "MODEM") == 0) return "RADIO";
    if (strcmp(upper_name, "SYSTEM") == 0) return "FACTORYFS";
    if (strcmp(upper_name, "DBDATA") == 0) return "PARAM";

    return NULL; // Could not determine partition
}
// Global state
static int initialized = 0;
static int device_detected = 0;
static PitInfo* pit_info = NULL;
static ProgressCallback progress_cb = NULL;
static LogCallback log_cb = NULL;

// Forward declarations
static void log_message(const char* msg, int type);
static int report_progress(float progress, const char* status);

// Initialize Heimdall
int heimdall_init(void) {
    if (initialized) return 0;
    
    log_message("Initializing Heimdall...", 0);
    
    // Initialize USB subsystem
    if (usb_init() != 0) {
        log_message("USB initialization failed", 2);
        return -1;
    }
    
    // Initialize file I/O
    if (fileio_init() != 0) {
        log_message("File I/O initialization failed", 2);
        return -1;
    }
    
    initialized = 1;
    log_message("Heimdall initialized successfully", 1);
    
    return 0;
}

// Cleanup
void heimdall_cleanup(void) {
    log_message("Cleaning up Heimdall...", 0);
    
    if (pit_info) {
        free(pit_info);
        pit_info = NULL;
    }
    
    flash_cleanup();
    usb_cleanup();
    fileio_cleanup();
    
    initialized = 0;
    log_message("Heimdall cleanup complete", 1);
}

// Detect Samsung device
int heimdall_detect_device(void) {
    if (!initialized) {
        log_message("Heimdall not initialized", 2);
        return -1;
    }
    
    report_progress(0.1f, "Scanning USB devices");
    
    // Scan for Samsung devices
    int device_count = usb_scan_devices();
    if (device_count <= 0) {
        log_message("No USB devices found", 2);
        return -1;
    }
    
    report_progress(0.3f, "Checking for Samsung");
    
    // Check each device
    for (int i = 0; i < device_count; i++) {
        uint16_t vid, pid;
        if (usb_get_device_info(i, &vid, &pid) == 0) {
            if (heimdall_is_samsung_device(vid, pid)) {
                report_progress(0.6f, "Found Samsung device");
                
                // Try to open device
                if (usb_open_device(i) == 0) {
                    device_detected = 1;
                    report_progress(1.0f, "Device opened");
                    
                    char msg[64];
                    snprintf(msg, sizeof(msg), 
                            "Device: %04X:%04X", vid, pid);
                    log_message(msg, 1);
                    
                    return 0;
                }
            }
        }
    }
    
    report_progress(0.0f, "No Samsung device found");
    log_message("No Samsung device in download mode found", 2);
    log_message("Connect device with Vol Down+Home+Power", 3);
    
    return -1;
}

// Load PIT file
int heimdall_load_pit(const char* filename) {
    if (!device_detected) {
        log_message("No device detected", 2);
        return -1;
    }
    
    report_progress(0.0f, "Loading PIT file");
    
    // Free previous PIT if any
    if (pit_info) {
        free(pit_info);
        pit_info = NULL;
    }
    
    // Allocate new PIT structure
    pit_info = malloc(sizeof(PitInfo));
    if (!pit_info) {
        log_message("Memory allocation failed", 2);
        return -1;
    }
    
    memset(pit_info, 0, sizeof(PitInfo));
    
    report_progress(0.3f, "Reading PIT data");
    
    // Read PIT file
    uint32_t file_size;
    uint8_t* pit_data = fileio_read_file(filename, &file_size);
    if (!pit_data) {
        log_message("Failed to read PIT file", 2);
        free(pit_info);
        pit_info = NULL;
        return -1;
    }
    
    report_progress(0.6f, "Parsing PIT");
    
    // Parse PIT file
    if (pit_parse(pit_data, file_size, pit_info) != 0) {
        log_message("Failed to parse PIT file", 2);
        free(pit_data);
        free(pit_info);
        pit_info = NULL;
        return -1;
    }
    
    free(pit_data);
    
    report_progress(1.0f, "PIT loaded");
    
    char msg[128];
    snprintf(msg, sizeof(msg), 
            "Loaded PIT with %d partitions", pit_info->entry_count);
    log_message(msg, 1);
    
    return 0;
}

// Get PIT info
PitInfo* heimdall_get_pit_info(void) {
    return pit_info;
}

// Determine partition from filename
const char* heimdall_determine_partition(const char* filename) {
    if (!filename) return NULL;
    
    const char* ext = strrchr(filename, '.');
    if (!ext) return NULL;
    
    // Check file extension
    if (strcasecmp(ext, ".img") == 0) {
        const char* name = strrchr(filename, '/');
        if (!name) name = filename;
        else name++;
        
        // Check for common partition names in filename
        if (strstr(name, "recovery") || strstr(name, "RECOVERY")) {
            return "RECOVERY";
        } else if (strstr(name, "boot") || strstr(name, "BOOT")) {
            return "BOOT";
        } else if (strstr(name, "system") || strstr(name, "SYSTEM")) {
            return "SYSTEM";
        } else if (strstr(name, "cache") || strstr(name, "CACHE")) {
            return "CACHE";
        } else if (strstr(name, "modem") || strstr(name, "MODEM")) {
            return "MODEM";
        } else if (strstr(name, "hidden") || strstr(name, "HIDDEN")) {
            return "HIDDEN";
        }
    } else if (strcasecmp(ext, ".bin") == 0) {
        // .bin files are usually modem or param
        return "MODEM";
    } else if (strcasecmp(ext, ".mbn") == 0) {
        return "MODEM";
    }
    
    return NULL;
}

// Flash file to partition
int heimdall_flash_file(const char* filename, const char* partition, 
                       ProgressCallback callback) {
    if (!device_detected) {
        log_message("No device detected", 2);
        return -1;
    }
    
    if (!filename || !partition) {
        log_message("Invalid parameters", 2);
        return -1;
    }
    
    // Set progress callback
    progress_cb = callback;
    
    // Start flash operation
    int result = flash_file(filename, partition, report_progress);
    
    // Clear callback
    progress_cb = NULL;
    
    return result;
}

// Reboot device
int heimdall_reboot(void) {
    if (!device_detected) {
        log_message("No device detected", 2);
        return -1;
    }
    
    log_message("Sending reboot command...", 0);
    
    // Send reboot command via USB
    uint8_t reboot_cmd[] = {0x0F, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    if (usb_send_bulk(reboot_cmd, sizeof(reboot_cmd)) != sizeof(reboot_cmd)) {
        log_message("Failed to send reboot command", 2);
        return -1;
    }
    
    log_message("Reboot command sent successfully", 1);
    
    // Close device (it will disconnect)
    usb_close_device();
    device_detected = 0;
    
    return 0;
}

// Download PIT from device
int heimdall_download_pit(void) {
    if (!device_detected) {
        log_message("No device detected", 2);
        return -1;
    }
    
    report_progress(0.0f, "Requesting PIT from device");
    
    // Request PIT from device
    uint8_t pit_request[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    if (usb_send_bulk(pit_request, sizeof(pit_request)) != sizeof(pit_request)) {
        log_message("Failed to request PIT", 2);
        return -1;
    }
    
    report_progress(0.3f, "Receiving PIT data");
    
    // Receive PIT data
    uint8_t* pit_data = NULL;
    uint32_t pit_size = 0;
    
    if (usb_receive_bulk(&pit_data, &pit_size) != 0) {
        log_message("Failed to receive PIT", 2);
        return -1;
    }
    
    report_progress(0.7f, "Saving PIT file");
    
    // Save PIT to SD card
    if (fileio_write_file("sd:/downloaded.pit", pit_data, pit_size) != 0) {
        log_message("Failed to save PIT file", 2);
        free(pit_data);
        return -1;
    }
    
    free(pit_data);
    
    report_progress(1.0f, "PIT downloaded");
    log_message("PIT downloaded to sd:/downloaded.pit", 1);
    
    return 0;
}

// Print PIT information
int heimdall_print_pit(void) {
    if (!pit_info) {
        log_message("No PIT loaded", 2);
        return -1;
    }
    
    log_message("=== PIT Information ===", 0);
    
    char info[256];
    snprintf(info, sizeof(info), 
            "Device: %s", pit_info->device_name);
    log_message(info, 0);
    
    snprintf(info, sizeof(info), 
            "Partitions: %d", pit_info->entry_count);
    log_message(info, 0);
    
    log_message("", 0);
    log_message("Partition List:", 0);
    
    for (uint32_t i = 0; i < pit_info->entry_count; i++) {
        PitEntry* entry = &pit_info->entries[i];
        
        snprintf(info, sizeof(info),
                "%2d. %-20s Size: %8d blocks",
                i + 1, entry->partition_name, entry->block_count);
        log_message(info, 0);
    }
    
    log_message("======================", 0);
    
    return 0;
}

// Set progress callback
void heimdall_set_progress_callback(ProgressCallback cb) {
    progress_cb = cb;
}

// Set log callback
void heimdall_set_log_callback(LogCallback cb) {
    log_cb = cb;
}

// Verify file integrity
int heimdall_verify_file(const char* filename) {
    report_progress(0.0f, "Verifying file");
    
    uint32_t file_size;
    uint8_t* data = fileio_read_file(filename, &file_size);
    if (!data) {
        log_message("Failed to read file for verification", 2);
        return -1;
    }
    
    report_progress(0.5f, "Calculating checksum");
    
    uint32_t checksum = heimdall_calculate_checksum(data, file_size);
    free(data);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "Checksum: 0x%08X", checksum);
    log_message(msg, 0);
    
    report_progress(1.0f, "Verification complete");
    
    return 0;
}

// Calculate checksum (simple XOR)
uint32_t heimdall_calculate_checksum(const uint8_t* data, uint32_t length) {
    uint32_t checksum = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 31);
    }
    
    return checksum;
}

// Check if device is Samsung
int heimdall_is_samsung_device(uint16_t vid, uint16_t pid) {
    // Samsung Vendor ID
    if (vid != 0x04E8) {
        return 0;
    }
    
    // Common Samsung Download Mode PIDs
    switch(pid) {
        case 0x6601: // Galaxy S series
        case 0x685D: // Many Samsung devices
        case 0x6860: // Galaxy devices
        case 0x68C3: // Note series
        case 0x68C4: // Tab series
        case 0x68DA: // A series
        case 0x68DB: // J series
            return 1;
        default:
            return 0;
    }
}

// Internal logging
static void log_message(const char* msg, int type) {
    if (log_cb) {
        log_cb(msg, type);
    }
}

// Internal progress reporting
static int report_progress(float progress, const char* status) {
    if (progress_cb) {
        return progress_cb(progress, status);
    }
    return 1; // Continue by default
}
