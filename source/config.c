#include "config.h"
#include <string.h>

// Matches the struct in your main.c
typedef struct {
    int state;       // We'll store the state as int
    int prev_state;
    int device_connected;
    int pit_loaded;
    char current_file[256];
    float flash_progress;
    char status_text[128];
    int auto_reboot;
    int verify_flash;
    int safe_mode;
} ConfigData;

#define CONFIG_PATH "sd:/heimdall.cfg"

void config_load(void* app_ptr) {
    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) return; // Use defaults if file doesn't exist

    ConfigData loaded;
    if (fread(&loaded, sizeof(ConfigData), 1, f) == 1) {
        // We only want to restore the settings, not the current state
        ConfigData* app = (ConfigData*)app_ptr;
        app->auto_reboot = loaded.auto_reboot;
        app->verify_flash = loaded.verify_flash;
        app->safe_mode = loaded.safe_mode;
    }
    fclose(f);
}

void config_save(void* app_ptr) {
    FILE *f = fopen(CONFIG_PATH, "wb");
    if (!f) return;

    ConfigData* app = (ConfigData*)app_ptr;
    fwrite(app, sizeof(ConfigData), 1, f);
    fclose(f);
}
