#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wiiuse/wpad.h>
#include <gccore.h>
#include "gui.h"

static void* xfb = NULL;
static GXRModeObj* rmode = NULL;

void gui_init(void) {
    // Initialize video
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

    // Initialize controllers
    WPAD_Init();
}

void gui_cleanup(void) {
    // Basic cleanup
}

void gui_show_message(const char* title, const char* message) {
    printf("\n[%s]\n%s\n", title, message);
}

void gui_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

int gui_get_selected(void) {
    // Placeholder: Returns 0 (first item) or checks controller
    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);
    if (pressed & WPAD_BUTTON_HOME) exit(0);
    return 0; 
}

void gui_set_progress(float percent) {
    // Simple ASCII progress bar
    int barWidth = 50;
    printf("\r[");
    int pos = barWidth * percent;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d %%", (int)(percent * 100.0));
}
