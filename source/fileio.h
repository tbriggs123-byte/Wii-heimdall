// source/fileio.h
#ifndef FILEIO_H
#define FILEIO_H

#include <stdint.h>

// File I/O functions
int fileio_init(void);
void fileio_cleanup(void);
uint8_t* fileio_read_file(const char* filename, uint32_t* length);
int fileio_write_file(const char* filename, const uint8_t* data, uint32_t length);
int fileio_file_exists(const char* filename);
uint32_t fileio_get_file_size(const char* filename);
int fileio_list_files(const char* directory, char*** files, int* count);
void fileio_free_list(char** files, int count);
int fileio_create_directory(const char* directory);
int fileio_delete_file(const char* filename);
int fileio_copy_file(const char* src, const char* dst);

// SD card specific
int fileio_get_sd_free_space(uint64_t* free_bytes);
int fileio_get_sd_total_space(uint64_t* total_bytes);
int fileio_is_sd_present(void);

#endif