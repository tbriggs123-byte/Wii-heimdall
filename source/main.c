// source/main.c
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gui.h"
#include "heimdall.h"
#include "config.h"

// Application state
typedef enum {
    STATE_MAIN_MENU,
    STATE_DEVICE_DETECT,
    STATE_PIT_LOAD,
    STATE_FLASHING,
    STATE_REBOOT,
    STATE_SETTINGS
} AppState;

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

// State machine handlers
void handle_main_menu(u32 pressed) {
    if (pressed & WPAD_BUTTON_A) {
        switch(gui_get_selected()) {
            case 0: // Detect Device
                app.state = STATE_DEVICE_DETECT;
                break;
            case 1: // Load PIT
                app.state = STATE_PIT_LOAD;
                break;
            case 2: // Flash Recovery
                strcpy(app.current_file, "sd:/recovery.img");
                app.state = STATE_FLASHING;
                break;
            case 3: // Flash System
                strcpy(app.current_file, "sd:/system.img");
                app.state = STATE_FLASHING;
                break;
            case 4: // Flash Boot
                strcpy(app.current_file, "sd:/boot.img");
                app.state = STATE_FLASHING;
                break;
            case 5: // Flash Cache
                strcpy(app.current_file, "sd:/cache.img");
                app.state = STATE_FLASHING;
                break;
            case 6: // Flash Modem
                strcpy(app.current_file, "sd:/modem.bin");
                app.state = STATE_FLASHING;
                break;
            case 7: // Reboot Device
                app.state = STATE_REBOOT;
                break;
            case 8: // Settings
                app.state = STATE_SETTINGS;
                break;
            case 9: // Exit
                running = 0;
                break;
        }
    }
}

void handle_device_detect(void) {
    gui_show_message("Detecting Samsung device...", MSG_INFO);
    
    if (heimdall_detect_device() == 0) {
        app.device_connected = 1;
        gui_show_message("Device detected successfully!", MSG_SUCCESS);
        gui_log("Samsung device found in download mode");
    } else {
        gui_show_message("No Samsung device found", MSG_ERROR);
        gui_log("Connect device in download mode (Vol Down+Home+Power)");
    }
    
    app.state = STATE_MAIN_MENU;
}

void handle_pit_load(void) {
    gui_show_message("Loading PIT file...", MSG_INFO);
    
    if (heimdall_load_pit("sd:/pit.pit") == 0) {
        app.pit_loaded = 1;
        gui_show_message("PIT file loaded successfully", MSG_SUCCESS);
        gui_log("PIT file parsed successfully");
        
        // Display PIT info
        PitInfo* pit = heimdall_get_pit_info();
        if (pit) {
            char info[256];
            snprintf(info, sizeof(info), 
                    "PIT: %d partitions, Device: %s",
                    pit->partition_count, pit->device_name);
            gui_log(info);
        }
    } else {
        gui_show_message("Failed to load PIT file", MSG_ERROR);
        gui_log("Place pit.pit on SD card root");
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
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Flashing %s to %s...", 
             filename, partition);
    gui_show_message(msg, MSG_INFO);
    
    // Start flash with progress callback
    int result = heimdall_flash_file(filename, partition, 
        [](float progress, const char* status) {
            app.flash_progress = progress;
            strncpy(app.status_text, status, sizeof(app.status_text)-1);
            gui_set_progress(progress, status);
            return 1; // Continue
        }
    );
    
    if (result == 0) {
        gui_show_message("Flash completed successfully!", MSG_SUCCESS);
        gui_log("Flash operation completed");
        
        if (app.auto_reboot) {
            app.state = STATE_REBOOT;
        } else {
            app.state = STATE_MAIN_MENU;
        }
    } else {
        gui_show_message("Flash failed!", MSG_ERROR);
        gui_log("Flash operation failed");
        app.state = STATE_MAIN_MENU;
    }
    
    app.flash_progress = 0;
}

void handle_reboot(void) {
    gui_show_message("Rebooting device...", MSG_INFO);
    
    if (heimdall_reboot() == 0) {
        gui_show_message("Reboot command sent", MSG_SUCCESS);
        gui_log("Device should reboot now");
    } else {
        gui_show_message("Failed to send reboot", MSG_ERROR);
    }
    
    app.state = STATE_MAIN_MENU;
}

void handle_settings(u32 pressed) {
    if (pressed & WPAD_BUTTON_A) {
        int selected = gui_get_selected();
        switch(selected) {
            case 0: // Toggle Auto Reboot
                app.auto_reboot = !app.auto_reboot;
                gui_log(app.auto_reboot ? 
                       "Auto reboot enabled" : "Auto reboot disabled");
                break;
            case 1: // Toggle Verify
                app.verify_flash = !app.verify_flash;
                gui_log(app.verify_flash ? 
                       "Flash verification enabled" : "Flash verification disabled");
                break;
            case 2: // Toggle Safe Mode
                app.safe_mode = !app.safe_mode;
                gui_log(app.safe_mode ? 
                       "Safe mode enabled" : "Safe mode disabled");
                break;
            case 3: // Save Settings
                config_save(&app);
                gui_show_message("Settings saved", MSG_SUCCESS);
                break;
            case 4: // Back
                app.state = STATE_MAIN_MENU;
                break;
        }
    }
    
    if (pressed & WPAD_BUTTON_B) {
        app.state = STATE_MAIN_MENU;
    }
}

// Main loop
int main(int argc, char **argv) {
    // Initialize systems
    VIDEO_Init();
    WPAD_Init();
    
    if (!fatInitDefault()) {
        // Try alternative initialization
        fatInitDefault();
    }
    
    // Initialize application
    memset(&app, 0, sizeof(app));
    app.state = STATE_MAIN_MENU;
    app.safe_mode = 1; // Default safe mode on
    
    // Load configuration
    config_load(&app);
    
    // Initialize GUI
    gui_init();
    
    // Initialize Heimdall
    if (heimdall_init() != 0) {
        gui_show_message("Heimdall initialization failed", MSG_ERROR);
    }
    
    // Main application loop
    while(running) {
        // Handle input
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        
        // Handle HOME button (exit)
        if (pressed & WPAD_BUTTON_HOME) {
            running = 0;
            break;
        }
        
        // State machine
        switch(app.state) {
            case STATE_MAIN_MENU:
                gui_show_main_menu(app.device_connected, app.pit_loaded);
                handle_main_menu(pressed);
                break;
                
            case STATE_DEVICE_DETECT:
                handle_device_detect();
                break;
                
            case STATE_PIT_LOAD:
                handle_pit_load();
                break;
                
            case STATE_FLASHING:
                handle_flashing();
                break;
                
            case STATE_REBOOT:
                handle_reboot();
                break;
                
            case STATE_SETTINGS:
                gui_show_settings(app.auto_reboot, app.verify_flash, app.safe_mode);
                handle_settings(pressed);
                break;
        }
        
        // Update GUI
        gui_update();
        
        // Wait for next frame
        VIDEO_WaitVSync();
    }
    
    // Cleanup
    heimdall_cleanup();
    gui_cleanup();
    
    // Save configuration
    config_save(&app);
    
    return 0;
}