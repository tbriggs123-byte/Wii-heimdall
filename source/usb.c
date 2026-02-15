#include "usb.h"
#include <ogc/usb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

static int usb_initialized = 0;
static s32 usb_device_fd = -1;
static uint8_t usb_endpoint_out = 0x01;
static uint8_t usb_endpoint_in = 0x81;
static uint8_t* usb_buffer = NULL;
static uint32_t usb_buffer_size = 0x10000;

// --- Core Subsystem Management ---

int usb_init(void) {
    if (usb_initialized) return 0;
    if (USB_Initialize() < 0) return -1;
    
    // Allocate 32-byte aligned memory for Wii DMA hardware
    usb_buffer = memalign(32, usb_buffer_size);
    if (!usb_buffer) return -1;
    
    usb_initialized = 1;
    return 0;
}

void usb_cleanup(void) {
    usb_close_device();
    if (usb_buffer) {
        free(usb_buffer);
        usb_buffer = NULL;
    }
    usb_initialized = 0;
}

int usb_init_device(void) {
    if (usb_init() < 0) return -1;
    return usb_open_device(0);
}

// --- Device Handle Management ---

int usb_open_device(int index) {
    if (!usb_initialized) return -1;
    s32 result = USB_OpenDevice(index, 0, 0, &usb_device_fd);
    if (result < 0) return -1;
    return 0;
}

void usb_close_device(void) {
    if (usb_device_fd >= 0) {
        USB_CloseDevice(&usb_device_fd);
        usb_device_fd = -1;
    }
}

int usb_is_device_open(void) {
    return (usb_device_fd >= 0);
}

int usb_is_connected(void) {
    return (usb_device_fd >= 0);
}

// --- Low-Level I/O ---

int usb_send_bulk(const uint8_t* data, uint32_t length) {
    // FIX: Added check for usb_buffer to satisfy the memcpy warning
    if (usb_device_fd < 0 || !usb_buffer) return -1;
    
    uint32_t sent = 0;
    while (sent < length) {
        uint32_t chunk = length - sent;
        if (chunk > usb_buffer_size) chunk = usb_buffer_size;
        
        memcpy(usb_buffer, data + sent, chunk);
        
        s32 result = USB_WriteBlkMsg(usb_device_fd, usb_endpoint_out, chunk, usb_buffer);
        if (result != (s32)chunk) return -1;
        
        sent += chunk;
    }
    return 0;
}

// --- Samsung Protocol Support ---

int usb_send_samsung_cmd(uint8_t cmd, uint32_t param) {
    if (usb_device_fd < 0 || !usb_buffer) return -1;

    memset(usb_buffer, 0, 16);
    usb_buffer[0] = cmd;
    // Parameter often goes in offset 2 or 4 depending on cmd
    memcpy(usb_buffer + 4, &param, sizeof(param)); 

    return usb_send_bulk(usb_buffer, 16);
}

// --- Flashing Bridge ---

int usb_start_flash_session(const char* partition) {
    // Placeholder for Samsung's PIT/Flash handshake
    return 0; 
}

int usb_send_data(const uint8_t* data, uint32_t size) {
    return usb_send_bulk(data, size);
}

int usb_end_flash_session(void) {
    return 0;
}
