// source/usb.h
#ifndef USB_H
#define USB_H

#include <stdint.h>

// USB functions
int usb_init(void);
void usb_cleanup(void);
int usb_scan_devices(void);
int usb_get_device_info(int index, uint16_t* vid, uint16_t* pid);
int usb_open_device(int index);
void usb_close_device(void);
int usb_is_device_open(void);
int usb_send_bulk(const uint8_t* data, uint32_t length);
int usb_receive_bulk(uint8_t** data, uint32_t* length);
int usb_send_control(uint8_t request, uint16_t value, uint16_t index, 
                     uint8_t* data, uint16_t length);
int usb_set_configuration(uint8_t config);
int usb_claim_interface(uint8_t interface);
int usb_release_interface(uint8_t interface);

// Samsung specific
int usb_send_samsung_cmd(uint8_t cmd, uint32_t param);
int usb_enter_download_mode(void);
int usb_get_device_info_samsung(char* model, char* serial);

#endif