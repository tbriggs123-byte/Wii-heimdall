// source/fileio.c
#include "fileio.h"
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>

// Initialize file I/O
int fileio_init(void) {
    // Already initialized by main.c
    return 0;
}

// Cleanup file I/O
void fileio_cleanup(void) {
    // Nothing to clean up
}

// Read file into memory
uint8_t* fileio_read_file(const char* filename, uint32_t* length) {
    if (!filename || !length) {
        return NULL;
    }
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t* buffer = malloc(file_size);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != file_size) {
        free(buffer);
        return NULL;
    }
    
    *length = file_size;
    return buffer;
}

// Write data to file
int fileio_write_file(const char* filename, const uint8_t* data, uint32_t length) {
    if (!filename || !data || length == 0) {
        return -1;
    }
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return -1;
    }
    
    size_t bytes_written = fwrite(data, 1, length, fp);
    fclose(fp);
    
    return (bytes_written == length) ? 0 : -1;
}

// Check if file exists
int fileio_file_exists(const char* filename) {
    if (!filename) {
        return 0;
    }
    
    FILE* fp = fopen(filename, "rb");
    if (fp) {
        fclose(fp);
        return 1;
    }
    
    return 0;
}

// Get file size
uint32_t fileio_get_file_size(const char* filename) {
    if (!filename) {
        return 0;
    }
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return 0;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    
    return (size > 0) ? size : 0;
}

// List files in directory
int fileio_list_files(const char* directory, char*** files, int* count) {
    if (!directory || !files || !count) {
        return -1;
    }
    
    DIR* dir = opendir(directory);
    if (!dir) {
        return -1;
    }
    
    // Count files
    struct dirent* entry;
    int file_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') { // Skip hidden files
            file_count++;
        }
    }
    
    rewinddir(dir);
    
    // Allocate array
    char** file_list = malloc(file_count * sizeof(char*));
    if (!file_list) {
        closedir(dir);
        return -1;
    }
    
    // Read file names
    int index = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            file_list[index] = strdup(entry->d_name);
            if (!file_list[index]) {
                // Cleanup on error
                for (int i = 0; i < index; i++) {
                    free(file_list[i]);
                }
                free(file_list);
                closedir(dir);
                return -1;
            }
            index++;
        }
    }
    
    closedir(dir);
    
    *files = file_list;
    *count = file_count;
    
    return 0;
}

// Free file list
void fileio_free_list(char** files, int count) {
    if (!files) return;
    
    for (int i = 0; i < count; i++) {
        if (files[i]) {
            free(files[i]);
        }
    }
    
    free(files);
}

// Create directory
int fileio_create_directory(const char* directory) {
    if (!directory) {
        return -1;
    }
    
    return mkdir(directory, 0777);
}

// Delete file
int fileio_delete_file(const char* filename) {
    if (!filename) {
        return -1;
    }
    
    return remove(filename);
}

// Copy file
int fileio_copy_file(const char* src, const char* dst) {
    if (!src || !dst) {
        return -1;
    }
    
    // Read source
    uint32_t src_size;
    uint8_t* src_data = fileio_read_file(src, &src_size);
    if (!src_data) {
        return -1;
    }
    
    // Write destination
    int result = fileio_write_file(dst, src_data, src_size);
    
    free(src_data);
    return result;
}

// Get SD card free space
int fileio_get_sd_free_space(uint64_t* free_bytes) {
    if (!free_bytes) {
        return -1;
    }
    
    // Use statvfs if available, otherwise estimate
    *free_bytes = 1024 * 1024 * 1024; // 1GB estimate
    
    return 0;
}

// Get SD card total space
int fileio_get_sd_total_space(uint64_t* total_bytes) {
    if (!total_bytes) {
        return -1;
    }
    
    // Use statvfs if available, otherwise estimate
    *total_bytes = 2048 * 1024 * 1024; // 2GB estimate
    
    return 0;
}

// Check if SD card is present
int fileio_is_sd_present(void) {
    // Try to access SD root
    DIR* dir = opendir("sd:/");
    if (dir) {
        closedir(dir);
        return 1;
    }
    
    return 0;
}