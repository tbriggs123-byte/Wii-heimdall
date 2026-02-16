#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <gctypes.h>

// Core USB subsystem
int usb_init(void);
void usb_cleanup(void);

// Logic-level functions (ADD THESE: they fix your compiler errors)
int usb_init_device(void); 
int usb_start_flash_session(const char* partition);
int usb_send_data(const uint8_t* data, uint32_t size);
int usb_end_flash_session(void);

// Device management
int usb_scan_devices(void);
int usb_open_device(int index);
void usb_close_device(void);
int usb_is_device_open(void);
int usb_is_connected(void);

// Low-level IO
int usb_send_bulk(const uint8_t* data, uint32_t length);
int usb_receive_bulk(uint8_t** data, uint32_t* length);
int usb_send_control(uint8_t request, uint16_t value, uint16_t index, 
                     uint8_t* data, uint16_t length);

// Samsung specific
// Change this line in usb.h:
int usb_send_samsung_cmd(const char* cmd_str, uint32_t param);
int usb_enter_download_mode(void);
int usb_get_device_info_samsung(char* model, char* serial);

#endif
