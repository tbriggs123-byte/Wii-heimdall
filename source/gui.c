#include <gccore.h>   // Must be first to define lwp types
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wiiuse/wpad.h>
#include "gui.h"

static void* xfb = NULL;
static GXRModeObj* rmode = NULL;

void gui_init(void) {
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    WPAD_Init();
}

void gui_cleanup(void) {
    // Cleanup logic
}

// Fixed to match: void gui_show_message(const char* message, int type);
void gui_show_message(const char* message, int type) {
    printf("\n[MSG Type %d]: %s\n", type, message);
}

// Fixed to match: void gui_log(const char* message, int type);
void gui_log(const char* message, int type) {
    printf("Log(%d): %s\n", type, message);
}

int gui_get_selected(void) {
    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);
    if (pressed & WPAD_BUTTON_HOME) exit(0);
    return 0; 
}

// Fixed to match: void gui_set_progress(float progress, const char* label);
void gui_set_progress(float progress, const char* label) {
    int barWidth = 40;
    printf("\r%s [", label);
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d%%", (int)(progress * 100.0));
    fflush(stdout);
}
// Add these to source/gui.c

void gui_show_main_menu(int device_connected, int pit_loaded) {
    printf("\x1b[2;0H"); // Move cursor to top
    printf("--- Wii Heimdall ---\n");
    printf("Status: %s | PIT: %s\n\n", 
           device_connected ? "Connected" : "Disconnected",
           pit_loaded ? "Loaded" : "Not Loaded");
    printf("1. Detect Device\n2. Load PIT\n3. Flash Recovery\n...");
}

void gui_show_settings(int auto_reboot, int verify, int safe_mode) {
    printf("\x1b[2;0H");
    printf("--- Settings ---\n");
    printf("Auto-Reboot: %s\n", auto_reboot ? "ON" : "OFF");
    printf("Verify:      %s\n", verify ? "ON" : "OFF");
}

void gui_render(void) {
    // On the Wii console, printf renders immediately, 
    // but we flush to be sure.
    fflush(stdout);
}
