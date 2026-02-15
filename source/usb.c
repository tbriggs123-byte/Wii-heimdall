#include "usb.h"
#include <ogc/usb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

static int usb_initialized = 0;
static s32 usb_device_fd = -1; // This stores the actual handle
static uint8_t usb_endpoint_out = 0x01;
static uint8_t usb_endpoint_in = 0x81;
static uint8_t* usb_buffer = NULL;
static uint32_t usb_buffer_size = 0x10000;

int usb_init(void) {
    if (usb_initialized) return 0;
    if (USB_Initialize() < 0) return -1;
    
    usb_buffer = memalign(32, usb_buffer_size);
    if (!usb_buffer) return -1;
    
    usb_initialized = 1;
    return 0;
}

int usb_open_device(int index) {
    if (!usb_initialized) return -1;
    
    // Fix 1: USB_OpenDevice needs 4 arguments and returns result in the 4th
    // s32 USB_OpenDevice(s32 device_id, u16 vid, u16 pid, s32 *fd);
    s32 result = USB_OpenDevice(index, 0, 0, &usb_device_fd);
    if (result < 0) return -1;

    return 0;
}

void usb_close_device(void) {
    if (usb_device_fd >= 0) {
        // Fix 2: USB_CloseDevice expects a pointer to the FD
        USB_CloseDevice(&usb_device_fd);
        usb_device_fd = -1;
    }
}

int usb_send_bulk(const uint8_t* data, uint32_t length) {
    if (usb_device_fd < 0) return -1;
    
    uint32_t sent = 0;
    while (sent < length) {
        uint32_t chunk = length - sent;
        if (chunk > usb_buffer_size) chunk = usb_buffer_size;
        
        memcpy(usb_buffer, data + sent, chunk);
        
        // Fix 3: libogc uses "BlkMsg" not "BulkMsg"
        s32 result = USB_WriteBlkMsg(usb_device_fd, usb_endpoint_out, chunk, usb_buffer);
        if (result != (s32)chunk) return -1;
        
        sent += chunk;
    }
    return 0;
}

int usb_receive_bulk(uint8_t** data, uint32_t* length) {
    if (usb_device_fd < 0 || !data || !length) return -1;
    
    // Fix 4: libogc uses "BlkMsg" not "BulkMsg"
    s32 result = USB_ReadBlkMsg(usb_device_fd, usb_endpoint_in, usb_buffer_size, usb_buffer);
    if (result < 0) return -1;

    *data = malloc(result);
    if (!*data) return -1;

    memcpy(*data, usb_buffer, result);
    *length = (uint32_t)result;
    return 0;
}

// --- Logic Bridge (Fixed Redefinitions) ---

int usb_start_flash_session(const char* partition) {
    uint8_t cmd[64];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x11; 
    strncpy((char*)cmd + 1, partition, 31);
    return usb_send_bulk(cmd, 64);
}

// Fix 5: Match 'const' qualifier from usb.h exactly
int usb_send_data(const uint8_t* data, uint32_t size) {
    return usb_send_bulk(data, size);
}

int usb_end_flash_session(void) {
    uint8_t cmd[16];
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = 0x12;
    return usb_send_bulk(cmd, 16);
}

int usb_is_connected(void) {
    return (usb_device_fd >= 0);
}
