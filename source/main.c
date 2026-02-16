#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "heimdall.h"
#include "config.h"

// --- State Machine Definitions ---
typedef enum {
    STATE_MAIN_MENU,
    STATE_DEVICE_DETECT,
    STATE_PIT_LOAD,
    STATE_FLASHING,
    STATE_REBOOT,
    STATE_SETTINGS
} AppState;

// Note: Ensure this struct matches what you have in config.h
typedef struct {
    AppState state;
    AppState prev_state;
    int device_connected;
    int pit_loaded;
    char current_file[256];
    float flash_progress;
    char status_text[128];
    int auto_reboot;
    int verify_flash;
    int safe_mode;
} AppData;

static AppData app;
static int running = 1;

// --- Callback for Flashing Progress ---
int on_flash_progress(float progress, const char* status) {
    app.flash_progress = progress;
    if (status) {
        strncpy(app.status_text, status, sizeof(app.status_text)-1);
    }
    
    gui_set_progress(progress, status);
    return 1; 
}

// --- State Machine Handlers ---

void handle_main_menu(u32 pressed) {
    if (pressed & WPAD_BUTTON_A) {
        switch(gui_get_selected()) {
            case 0: app.state = STATE_DEVICE_DETECT; break;
            case 1: app.state = STATE_PIT_LOAD; break;
            case 2: strcpy(app.current_file, "sd:/recovery.img"); app.state = STATE_FLASHING; break;
            case 3: strcpy(app.current_file, "sd:/system.img"); app.state = STATE_FLASHING; break;
            case 4: strcpy(app.current_file, "sd:/boot.img"); app.state = STATE_FLASHING; break;
            case 5: strcpy(app.current_file, "sd:/cache.img"); app.state = STATE_FLASHING; break;
            case 6: strcpy(app.current_file, "sd:/modem.bin"); app.state = STATE_FLASHING; break;
            case 7: app.state = STATE_REBOOT; break;
            case 8: app.state = STATE_SETTINGS; break;
            case 9: running = 0; break;
        }
    }
}

void handle_device_detect(void) {
    gui_show_message("Detecting Samsung device...", MSG_INFO);
    
    if (heimdall_detect_device() == 0) {
        app.device_connected = 1;
        gui_show_message("Device detected successfully!", MSG_SUCCESS);
        gui_log("Samsung device found in download mode", MSG_SUCCESS);
    } else {
        gui_show_message("No Samsung device found", MSG_ERROR);
        gui_log("Connect device in download mode (Vol Down+Home+Power)", MSG_WARNING);
    }
    app.state = STATE_MAIN_MENU;
}

void handle_pit_load(void) {
    gui_show_message("Loading PIT file...", MSG_INFO);
    
    if (heimdall_load_pit("sd:/pit.pit") == 0) {
        app.pit_loaded = 1;
        gui_show_message("PIT file loaded successfully", MSG_SUCCESS);
        PitInfo* pit = heimdall_get_pit_info();
        if (pit) {
            char info[256];
            snprintf(info, sizeof(info), "PIT: %u partitions, Device: %s", (unsigned int)pit->entry_count, pit->device_name);
            gui_log(info, MSG_INFO);
        }
    } else {
        gui_show_message("Failed to load PIT file", MSG_ERROR);
        gui_log("Place pit.pit on SD card root", MSG_ERROR);
    }
    app.state = STATE_MAIN_MENU;
}

void handle_flashing(void) {
    const char* filename = app.current_file;
    const char* partition = heimdall_determine_partition(filename);
    
    if (!partition) {
        gui_show_message("Unknown file type", MSG_ERROR);
        app.state = STATE_MAIN_MENU;
        return;
    }
    
    char msg[512]; 
    snprintf(msg, sizeof(msg), "Flashing %s to %s...", filename, partition);
    gui_show_message(msg, MSG_INFO);
    
    int result = heimdall_flash_file(filename, partition, on_flash_progress);
    
    if (result == 0) {
        gui_show_message("Flash completed successfully!", MSG_SUCCESS);
        app.state = app.auto_reboot ? STATE_REBOOT : STATE_MAIN_MENU;
    } else {
        gui_show_message("Flash failed!", MSG_ERROR);
        app.state = STATE_MAIN_MENU;
    }
    app.flash_progress = 0;
}

void handle_reboot(void) {
    gui_show_message("Rebooting device...", MSG_INFO);
    if (heimdall_reboot() == 0) {
        gui_show_message("Reboot command sent", MSG_SUCCESS);
    } else {
        gui_show_message("Failed to send reboot", MSG_ERROR);
    }
    app.state = STATE_MAIN_MENU;
}

void handle_settings(u32 pressed) {
    if (pressed & WPAD_BUTTON_A) {
        switch(gui_get_selected()) {
            case 0: app.auto_reboot = !app.auto_reboot; break;
            case 1: app.verify_flash = !app.verify_flash; break;
            case 2: app.safe_mode = !app.safe_mode; break;
            case 3: config_save(&app); gui_show_message("Settings saved", MSG_SUCCESS); break;
            case 4: app.state = STATE_MAIN_MENU; break;
        }
    }
    if (pressed & WPAD_BUTTON_B) app.state = STATE_MAIN_MENU;
}

// --- Main Loop ---

int main(int argc, char **argv) {
    // 1. Force IOS58 for USB 2.0 Support
    if (IOS_GetVersion() != 58) {
        IOS_ReloadIOS(58);
    }
    
    // 2. Start Video and Input first (so we can see error messages)
    gui_init();
    
    // 3. Initialize SD Card
    if (!fatInitDefault()) {
        gui_show_message("SD Card failed to initialize!", MSG_ERROR);
    }
    
    // 4. Set Initial State
    memset(&app, 0, sizeof(app));
    app.state = STATE_MAIN_MENU;
    app.safe_mode = 1;
    
    // Load existing settings if any
    config_load(&app);
    
    // 5. Initialize USB Subsystem
    if (heimdall_init() != 0) {
        gui_show_message("USB init failed! Connect to Port 0.", MSG_ERROR);
    }
    
    while(running) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        
        if (pressed & WPAD_BUTTON_HOME) break;
        
        switch(app.state) {
            case STATE_MAIN_MENU:
                gui_show_main_menu(app.device_connected, app.pit_loaded);
                handle_main_menu(pressed);
                break;
            case STATE_DEVICE_DETECT: handle_device_detect(); break;
            case STATE_PIT_LOAD:      handle_pit_load(); break;
            case STATE_FLASHING:      handle_flashing(); break;
            case STATE_REBOOT:        handle_reboot(); break;
            case STATE_SETTINGS:
                gui_show_settings(app.auto_reboot, app.verify_flash, app.safe_mode);
                handle_settings(pressed);
                break;
        }
        
        gui_render(); 
        VIDEO_WaitVSync();
    }
    
    // 6. Cleanup
    heimdall_cleanup();
    gui_cleanup();
    
    return 0;
}
