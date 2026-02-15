// source/usb.c
#include "usb.h"
#include <ogc/usb.h>
#include <ogc/ipc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// USB state
static int usb_initialized = 0;
static s32 usb_device_fd = -1;
static uint8_t usb_endpoint_out = 0x01;
static uint8_t usb_endpoint_in = 0x81;
static uint8_t* usb_buffer = NULL;
static uint32_t usb_buffer_size = 0x10000; // 64KB

// Initialize USB subsystem
int usb_init(void) {
    if (usb_initialized) return 0;
    
    // Initialize USB library
    if (USB_Initialize() < 0) {
        return -1;
    }
    
    // Allocate buffer
    usb_buffer = memalign(32, usb_buffer_size);
    if (!usb_buffer) {
        USB_Deinitialize();
        return -1;
    }
    
    usb_initialized = 1;
    return 0;
}

// Cleanup USB
void usb_cleanup(void) {
    if (usb_device_fd >= 0) {
        USB_CloseDevice(usb_device_fd);
        usb_device_fd = -1;
    }
    
    if (usb_buffer) {
        free(usb_buffer);
        usb_buffer = NULL;
    }
    
    if (usb_initialized) {
        USB_Deinitialize();
        usb_initialized = 0;
    }
}

// Scan for USB devices
int usb_scan_devices(void) {
    if (!usb_initialized) return -1;
    
    return USB_GetDeviceList();
}

// Get device VID/PID
int usb_get_device_info(int index, uint16_t* vid, uint16_t* pid) {
    if (!usb_initialized) return -1;
    
    usb_device_entry* entry = USB_GetDeviceEntry(index);
    if (!entry) return -1;
    
    if (vid) *vid = entry->vid;
    if (pid) *pid = entry->pid;
    
    return 0;
}

// Open USB device
int usb_open_device(int index) {
    if (!usb_initialized) return -1;
    
    // Close existing device if open
    if (usb_device_fd >= 0) {
        USB_CloseDevice(usb_device_fd);
        usb_device_fd = -1;
    }
    
    // Open new device
    usb_device_fd = USB_OpenDevice(index, 0, 0);
    if (usb_device_fd < 0) {
        return -1;
    }
    
    // Try to claim interface (Samsung uses interface 0)
    if (USB_ClaimInterface(usb_device_fd, 0) < 0) {
        USB_CloseDevice(usb_device_fd);
        usb_device_fd = -1;
        return -1;
    }
    
    return 0;
}

// Close USB device
void usb_close_device(void) {
    if (usb_device_fd >= 0) {
        USB_ReleaseInterface(usb_device_fd, 0);
        USB_CloseDevice(usb_device_fd);
        usb_device_fd = -1;
    }
}

// Check if device is open
int usb_is_device_open(void) {
    return (usb_device_fd >= 0);
}

// Send bulk data
int usb_send_bulk(const uint8_t* data, uint32_t length) {
    if (usb_device_fd < 0) return -1;
    
    // Split into chunks if necessary
    uint32_t sent = 0;
    while (sent < length) {
        uint32_t chunk = length - sent;
        if (chunk > usb_buffer_size) {
            chunk = usb_buffer_size;
        }
        
        // Copy to aligned buffer
        memcpy(usb_buffer, data + sent, chunk);
        
        // Send chunk
        s32 result = USB_WriteIntrMsg(usb_device_fd, usb_endpoint_out, 
                                      chunk, usb_buffer);
        if (result != chunk) {
            return -1;
        }
        
        sent += chunk;
    }
    
    return sent;
}

// Receive bulk data
int usb_receive_bulk(uint8_t** data, uint32_t* length) {
    if (usb_device_fd < 0 || !data || !length) return -1;
    
    // First, try to get size
    uint8_t size_buffer[16];
    s32 result = USB_ReadIntrMsg(usb_device_fd, usb_endpoint_in, 
                                 sizeof(size_buffer), size_buffer);
    if (result <= 0) {
        return -1;
    }
    
    // Parse size from response (simplified)
    uint32_t data_size = 0;
    if (result >= 4) {
        data_size = *(uint32_t*)size_buffer;
    }
    
    if (data_size == 0 || data_size > 0x100000) { // 1MB max
        return -1;
    }
    
    // Allocate buffer
    *data = malloc(data_size);
    if (!*data) {
        return -1;
    }
    
    // Copy initial data
    uint32_t received = 0;
    if (result > 4) {
        uint32_t copy_size = result - 4;
        if (copy_size > data_size) copy_size = data_size;
        memcpy(*data, size_buffer + 4, copy_size);
        received = copy_size;
    }
    
    // Receive remaining data
    while (received < data_size) {
        uint32_t to_receive = data_size - received;
        if (to_receive > usb_buffer_size) {
            to_receive = usb_buffer_size;
        }
        
        result = USB_ReadIntrMsg(usb_device_fd, usb_endpoint_in, 
                                 to_receive, usb_buffer);
        if (result <= 0) {
            free(*data);
            *data = NULL;
            return -1;
        }
        
        memcpy(*data + received, usb_buffer, result);
        received += result;
    }
    
    *length = received;
    return 0;
}

// Send control transfer
int usb_send_control(uint8_t request, uint16_t value, uint16_t index, 
                     uint8_t* data, uint16_t length) {
    if (usb_device_fd < 0) return -1;
    
    usb_ctrl_setup ctrl_setup;
    memset(&ctrl_setup, 0, sizeof(ctrl_setup));
    
    ctrl_setup.bmRequestType = USB_REQTYPE_TYPE_VENDOR | 
                               USB_REQTYPE_RECIPIENT_DEVICE;
    ctrl_setup.bRequest = request;
    ctrl_setup.wValue = value;
    ctrl_setup.wIndex = index;
    ctrl_setup.wLength = length;
    
    return USB_WriteCtrlMsg(usb_device_fd, &ctrl_setup, data);
}

// Set configuration
int usb_set_configuration(uint8_t config) {
    if (usb_device_fd < 0) return -1;
    
    return USB_SetConfiguration(usb_device_fd, config);
}

// Claim interface
int usb_claim_interface(uint8_t interface) {
    if (usb_device_fd < 0) return -1;
    
    return USB_ClaimInterface(usb_device_fd, interface);
}

// Release interface
int usb_release_interface(uint8_t interface) {
    if (usb_device_fd < 0) return -1;
    
    return USB_ReleaseInterface(usb_device_fd, interface);
}

// Samsung specific commands
int usb_send_samsung_cmd(uint8_t cmd, uint32_t param) {
    uint8_t buffer[16];
    memset(buffer, 0, sizeof(buffer));
    
    buffer[0] = cmd;
    buffer[1] = 0x00;
    *(uint32_t*)(buffer + 2) = param;
    
    return usb_send_bulk(buffer, 16);
}

// Enter download mode (if not already)
int usb_enter_download_mode(void) {
    // Send enter download mode command
    return usb_send_samsung_cmd(0x0F, 0x00000000);
}

// Get device info from Samsung
int usb_get_device_info_samsung(char* model, char* serial) {
    if (!model && !serial) return -1;
    
    // Request device info
    if (usb_send_samsung_cmd(0x3E, 0x00000000) != 0) {
        return -1;
    }
    
    // Receive response
    uint8_t* data = NULL;
    uint32_t length = 0;
    
    if (usb_receive_bulk(&data, &length) != 0) {
        return -1;
    }
    
    // Parse response (simplified)
    if (length >= 64) {
        if (model) {
            strncpy(model, (char*)data, 32);
            model[31] = '\0';
        }
        if (serial && length >= 96) {
            strncpy(serial, (char*)data + 64, 32);
            serial[31] = '\0';
        }
    }
    
    free(data);
    return 0;
}