#ifndef GUI_H
#define GUI_H

#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Message types
#define MSG_INFO     0
#define MSG_SUCCESS  1
#define MSG_ERROR    2
#define MSG_WARNING  3

// Colors (ARGB format)
#define COLOR_BACKGROUND   0x006666FF  // Blue
#define COLOR_TEXT         0xFFFFFFFF  // White
#define COLOR_TEXT_DARK    0xFF888888  // Gray
#define COLOR_HIGHLIGHT    0x00FF6600  // Orange
#define COLOR_SUCCESS      0x0000FF00  // Green
#define COLOR_ERROR        0x00FF0000  // Red
#define COLOR_WARNING      0x00FFFF00  // Yellow
#define COLOR_BUTTON_BG    0x333333FF  // Dark gray
#define COLOR_BUTTON_BORDER 0xFFFFFFFF // White
#define COLOR_PROGRESS_BG  0x222222FF  // Very dark gray
#define COLOR_PROGRESS_FILL 0x00FF00FF // Bright green
#define COLOR_LOG_BG       0x111111FF  // Almost black
#define COLOR_HEADER_BG    0x000033FF  // Dark blue

// Screen dimensions
#define SCREEN_WIDTH   640
#define SCREEN_HEIGHT  480
#define HEADER_HEIGHT  60
#define FOOTER_HEIGHT  40

// Button structure
typedef struct {
    int x, y;
    int width, height;
    char text[64];
    int enabled;
    int selected;
    void (*callback)(void);
} Button;

// Progress bar structure
typedef struct {
    int x, y;
    int width, height;
    float progress;
    char label[32];
} ProgressBar;

// Log entry structure
typedef struct {
    char text[256];
    int type;
    u32 timestamp;
} LogEntry;

// Menu structure
typedef struct {
    Button* buttons;
    int button_count;
    int selected_index;
    char title[64];
} Menu;

// GUI state structure
typedef struct {
    // Screen buffers
    void* framebuffer;
    GXRModeObj* video_mode;
    
    // Current menu
    Menu* current_menu;
    
    // Progress display
    ProgressBar progress_bar;
    
    // Log system
    LogEntry logs[20];
    int log_count;
    int log_scroll;
    
    // Message display
    char message[256];
    int message_type;
    u32 message_time;
    
    // Status display
    char status[128];
    
    // Device info
    char device_info[128];
    int device_connected;
    
    // File info
    char current_file[256];
    char current_partition[32];
    
    // Flags
    int needs_redraw;
    int exit_requested;
} GUIState;

// GUI initialization and management
void gui_init(void);
void gui_cleanup(void);
void gui_update(void);
void gui_render(void);
void gui_handle_input(u32 pressed);

// Menu management
Menu* gui_create_menu(const char* title);
void gui_free_menu(Menu* menu);
void gui_add_button(Menu* menu, const char* text, void(*callback)(void));
void gui_set_menu(Menu* menu);
void gui_show_main_menu(int device_connected, int pit_loaded);
void gui_show_settings(int auto_reboot, int verify_flash, int safe_mode);
void gui_show_file_browser(void);

// Button management
int gui_get_selected(void);
void gui_set_button_enabled(int index, int enabled);
void gui_select_button(int index);

// Progress display
void gui_set_progress(float progress, const char* label);
void gui_show_progress_screen(const char* operation, const char* details);

// Message display
void gui_show_message(const char* message, int type);
void gui_clear_message(void);

// Log system
void gui_log(const char* message, int type);
void gui_clear_logs(void);
void gui_scroll_logs(int direction);

// Status display
void gui_set_status(const char* status);
void gui_set_device_info(const char* info);
void gui_set_file_info(const char* filename, const char* partition);

// Drawing functions
void gui_draw_background(void);
void gui_draw_header(void);
void gui_draw_footer(void);
void gui_draw_button(Button* button);
void gui_draw_menu(Menu* menu);
void gui_draw_progress_bar(ProgressBar* pb);
void gui_draw_logs(void);
void gui_draw_message(void);
void gui_draw_status(void);
void gui_draw_device_info(void);
void gui_draw_file_info(void);
void gui_draw_text(int x, int y, const char* text, u32 color, int size);
void gui_draw_box(int x, int y, int w, int h, u32 color);
void gui_draw_border(int x, int y, int w, int h, u32 color, int thickness);

// Utility functions
void gui_center_text(int y, const char* text, u32 color, int size);
int gui_get_text_width(const char* text, int size);
void gui_wrap_text(char* dest, const char* src, int max_width, int size);

// Input handling
void gui_process_dpad(u32 pressed);
void gui_process_buttons(u32 pressed);

// Animation
void gui_pulse_effect(int* intensity);

#endif // GUI_H
