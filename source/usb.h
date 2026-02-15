// Logic-level functions (ADD THESE: they fix your compiler errors)
int usb_init_device(void); 
int usb_start_flash_session(const char* partition);
int usb_send_data(const uint8_t* data, uint32_t size);
int usb_end_flash_session(void);
