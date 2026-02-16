#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "usb.h"

#define SAMSUNG_VID 0x04E8
#define SAMSUNG_PID 0x685D

static s32 usb_device_fd = -1;
static uint8_t* usb_buffer = NULL; 
static const uint32_t BUFFER_SIZE = 0x10000;

// Hardware Endpoints for Samsung Download Mode
static u8 endpoint_out = 0x01;
static u8 endpoint_in = 0x81;

int usb_init(void) {
    if (USB_Initialize() < 0) return -1;
    if (!usb_buffer) {
        // Allocate 32-byte aligned memory for DMA
        usb_buffer = memalign(32, BUFFER_SIZE);
    }
    return (usb_buffer) ? 0 : -1;
}

int usb_open_device(int index) {
    // 1. Open the device handle
    s32 result = USB_OpenDevice(index, SAMSUNG_VID, SAMSUNG_PID, &usb_device_fd);
    if (result < 0) {
        result = USB_OpenDevice(index, SAMSUNG_VID, 0x68C0, &usb_device_fd);
    }

    if (result < 0) return -1;

    // 2. REAL INTERFACE CLAIMING
    // On the Wii, "claiming" is done by selecting the configuration 
    // and setting the alternate interface.
    
    u8 config = 0;
    // Get the first configuration
    if (USB_GetConfiguration(usb_device_fd, &config) < 0) {
        config = 1; // Default to 1 if read fails
    }

    if (USB_SetConfiguration(usb_device_fd, config) < 0) {
        return -2;
    }

    // Samsung uses Interface 0, AltSetting 0 for Odin/Heimdall protocol
    if (USB_SetAlternativeInterface(usb_device_fd, 0, 0) < 0) {
        // Some devices don't require this call, but it's safer to attempt
    }

    return 0;
}

// --- The Handshake ---

int usb_send_samsung_cmd(const char* cmd_str, uint32_t param) {
    if (usb_device_fd < 0) return -1;

    // Samsung protocol expects exactly 16 bytes
    memset(usb_buffer, 0, 16);
    memcpy(usb_buffer, cmd_str, strlen(cmd_str) > 16 ? 16 : strlen(cmd_str));
    
    // Pack parameter as Big Endian at the end of the 16-byte packet
    usb_buffer[12] = (param >> 24) & 0xFF;
    usb_buffer[13] = (param >> 16) & 0xFF;
    usb_buffer[14] = (param >> 8) & 0xFF;
    usb_buffer[15] = param & 0xFF;

    s32 res = USB_WriteBlkMsg(usb_device_fd, endpoint_out, 16, usb_buffer);
    return (res == 16) ? 0 : -1;
}

int usb_start_flash_session(const char* partition) {
    // This is the sequence Heimdall/Odin uses to "wake up" the phone
    if (usb_send_samsung_cmd("Odin", 0) < 0) return -1;
    
    // Request to begin PIT transmission
    if (usb_send_samsung_cmd("PITR", 0) < 0) return -2;
    
    // Select the target partition
    if (usb_send_samsung_cmd(partition, 0) < 0) return -3;

    return 0;
}

int usb_send_data(const uint8_t* data, uint32_t size) {
    if (usb_device_fd < 0) return -1;

    uint32_t sent = 0;
    while (sent < size) {
        uint32_t chunk = (size - sent > BUFFER_SIZE) ? BUFFER_SIZE : (size - sent);
        
        // Copy to aligned buffer
        memcpy(usb_buffer, data + sent, chunk);
        
        // Write to Bulk OUT endpoint
        s32 res = USB_WriteBlkMsg(usb_device_fd, endpoint_out, chunk, usb_buffer);
        if (res != (s32)chunk) return -1;
        
        sent += chunk;
    }
    return 0;
}
