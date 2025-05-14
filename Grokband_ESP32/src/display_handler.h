#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include "lvgl.h"

void display_init();
void display_update_loop(); // Call this in your main loop
void display_show_message(const char* message);
void display_show_quick_responses_ui(const char* responses[], int count, int selected_idx);
void display_clear();
void display_show_status(const char* status);

// Callback type for rotary encoder events if needed by display
typedef void (*input_event_cb_t)(int8_t direction, bool pressed); 
void display_set_input_callback(input_event_cb_t cb);


#endif // DISPLAY_HANDLER_H