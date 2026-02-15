// source/flash.h
#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

// Progress callback
typedef int (*FlashProgressCallback)(float progress, const char* status);

// Flash functions
int flash_init(void);
void flash_cleanup(void);
int flash_file(const char* filename, const char* partition, 
               FlashProgressCallback callback);
int flash_data(const uint8_t* data, uint32_t length, const char* partition,
               FlashProgressCallback callback);
int flash_verify(const char* filename, const char* partition);
int flash_abort(void);
int flash_is_busy(void);
float flash_get_progress(void);
const char* flash_get_status(void);

// Samsung flash protocol
int samsung_send_file_header(const char* filename, uint32_t file_size, 
                             uint32_t file_type);
int samsung_send_file_part(const uint8_t* data, uint32_t length, 
                           uint32_t sequence);
int samsung_send_file_end(uint32_t file_size, uint32_t checksum);
int samsung_wait_ack(void);
int samsung_send_pit(const uint8_t* pit_data, uint32_t pit_size);

#endif