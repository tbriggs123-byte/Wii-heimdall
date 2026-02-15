#include "usb.h"
#include <ogc/usb.h>
#include <ogc/ipc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

// USB state
static int usb_initialized = 0;
static s32 usb_device_fd = -1;
static uint8_t usb_endpoint_out = 0x01; // Standard for Samsung
static uint8_t usb_endpoint_in = 0x81;
static uint8_t* usb_buffer = NULL;
static uint32_t usb_buffer_size = 0x10000; // 64KB

// --- Subsystem Management ---

int usb_init(void) {
    if (usb_initialized) return 0;
    if (USB_Initialize() < 0) return -1;
    
    usb_buffer = memalign(32, usb_buffer_size);
    if (!usb_buffer) {
        USB_Deinitialize();
        return -1;
    }
    
    usb_initialized = 1;
    return 0;
}

void usb_cleanup(void) {
    usb_close_device();
    if (usb_buffer) {
        free(usb_buffer);
        usb_buffer = NULL;
    }
    if (usb_initialized) {
        USB_Deinitialize();
        usb_initialized = 0;
    }
}

// --- Device Connection ---

int usb_open_device(int index) {
    if (!usb_initialized) return -1;
    if (usb_device_fd >= 0) usb_close_device();

    // index 0 is typically the first USB port on the Wii
    usb_device_fd = USB_OpenDevice(index, 0, 0);
    if (usb_device_fd < 0) return -1;

    // Samsung devices require claiming interface 0
    if (USB_ClaimInterface(usb_device_fd, 0) < 0) {
        usb_close_device();
        return -1;
    }
    return 0;
}

void usb_close_device(void) {
    if (usb_device_fd >= 0) {
        USB_ReleaseInterface(usb_device_fd, 0);
        USB_CloseDevice(usb_device_fd);
        usb_device_fd = -1;
    }
}

int usb_is_device_open(void) {
    return (usb_device_fd >= 0);
}

// --- Data Transfer (Fixed to use Bulk) ---

int usb_send_bulk(const uint8_t* data, uint32_t length) {
    if (usb_device_fd < 0) return -1;
    
    uint32_t sent = 0;
    while (sent < length) {
        uint32_t chunk = length - sent;
        if (chunk > usb_buffer_size) chunk = usb_buffer_size;
        
        memcpy(usb_buffer, data + sent, chunk);
        
        // CRITICAL FIX: Changed from IntrMsg to BulkMsg
        s32 result = USB_WriteBulkMsg(usb_device_fd, usb_endpoint_out, chunk, usb_buffer);
        if (result != (s32)chunk) return -1;
        
        sent += chunk;
    }
    return 0;
}

int usb_receive_bulk(uint8_t** data, uint32_t* length) {
    if (usb_device_fd < 0 || !data || !length) return -1;
    
    // Samsung responses usually fit in one buffer for commands
    s32 result = USB_ReadBulkMsg(usb_device_fd, usb_endpoint_in, usb_buffer_size, usb_buffer);
    if (result < 0) return -1;

    *data = malloc(result);
    if (!*data) return -1;

    memcpy(*data, usb_buffer, result);
    *length = (uint32_t)result;
    return 0;
}

// --- Heimdall/Samsung Bridge Functions (Required by heimdall.c) ---

int usb_start_flash_session(const char* partition) {
    // Protocol command to start flashing a specific partition
    // Typically command 0x11 followed by partition name
    uint8_t cmd[64];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x11; // Samsung Start Session Command
    strncpy((char*)cmd + 1, partition, 32);
    
    return usb_send_bulk(cmd, 64);
}

int usb_send_data(uint8_t* data, size_t size) {
    return usb_send_bulk(data, (uint32_t)size);
}

int usb_end_flash_session(void) {
    // Protocol command to end session (Command 0x12)
    uint8_t cmd[16];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x12;
    return usb_send_bulk(cmd, 16);
}

int usb_is_connected(void) {
    return usb_is_device_open();
}
// Add these to the bottom of your existing source/usb.c

int usb_init_device(void) {
    if (usb_init() < 0) return -1;
    return usb_open_device(0); // Try to open the first Samsung device found
}

int usb_start_flash_session(const char* partition) {
    // Samsung Protocol: Start Flash (0x11) + Partition Name
    uint8_t cmd[64];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x11; 
    strncpy((char*)cmd + 1, partition, 31);
    return usb_send_bulk(cmd, 64);
}

int usb_send_data(const uint8_t* data, uint32_t size) {
    return usb_send_bulk(data, size);
}

int usb_end_flash_session(void) {
    // Samsung Protocol: End Flash (0x12)
    uint8_t cmd[16];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x12;
    return usb_send_bulk(cmd, 16);
}
