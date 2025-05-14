#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <stdint.h>

enum class InputEvent {
    NONE,
    ENCODER_UP,
    ENCODER_DOWN,
    ENCODER_PRESS
};

void input_init();
InputEvent input_get_event(); // Polling based
void input_rotary_loop(); // If using RotaryEncoder library's loop method

// For LVGL integration (if not polling)
typedef void (*lvgl_encoder_cb_t)(int8_t diff, lv_indev_state_t state);
void input_set_lvgl_cb(lvgl_encoder_cb_t cb);

int8_t get_encoder_diff();
bool is_encoder_pressed();
void clear_encoder_state();


#endif // INPUT_HANDLER_H