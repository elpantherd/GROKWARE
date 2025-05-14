#include "input_handler.h"
#include "config.h"
#include <RotaryEncoder.h>
#include "lvgl.h" // For lv_indev_state_t

// Setup a RotaryEncoder with 2 steps per detent and internal pullup resistors
// RotaryEncoder encoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, RotaryEncoder::LatchMode::FOUR3); // Example for specific encoder
// Simpler constructor:
RotaryEncoder encoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN);


volatile long last_encoder_pos = 0;
volatile bool encoder_button_pressed = false;
lvgl_encoder_cb_t g_lvgl_cb = nullptr;


// Interrupt service routines (ISRs) - keep them short!
#if defined(ESP32)
void IRAM_ATTR read_encoder_isr() {
    encoder.tick();
}
void IRAM_ATTR encoder_button_isr() {
    // Basic debouncing, you might need more sophisticated logic
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) { // 200ms debounce
        encoder_button_pressed = true; // Set flag, process in loop
        last_interrupt_time = interrupt_time;
    }
}
#endif


void input_init() {
    pinMode(ROTARY_ENCODER_SW_PIN, INPUT_PULLUP); // Assuming active low button
    
    #if defined(ESP_PLATFORM) // ESP32 specific
    // attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_A_PIN), read_encoder_isr, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_B_PIN), read_encoder_isr, CHANGE);
    // The RotaryEncoder library often handles its own tick if you call encoder.tick() regularly.
    // If it does not, you might need ISRs like above or use ESP32's PCNT peripheral.

    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_SW_PIN), encoder_button_isr, FALLING);
    #else
    // For other platforms, you might need different interrupt setup
    #endif
    
    encoder.setPosition(0); // Start position
    Serial.println("Input Handler Initialized");
}

void input_set_lvgl_cb(lvgl_encoder_cb_t cb) {
    g_lvgl_cb = cb;
}


// This function should be called by LVGL's input device read callback
// It simulates how LVGL would read an encoder
void lvgl_encoder_read_fake(lv_indev_drv_t * drv, lv_indev_data_t * data) {
    encoder.tick(); // Ensure encoder state is up-to-date
    long new_pos = encoder.getPosition();
    int diff = 0;

    if (new_pos > last_encoder_pos) {
        diff = 1;
    } else if (new_pos < last_encoder_pos) {
        diff = -1;
    }
    last_encoder_pos = new_pos; // Update for next comparison

    data->enc_diff = diff;

    if (encoder_button_pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        encoder_button_pressed = false; // Consume press
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


// If not using LVGL's input system directly, you can poll
InputEvent input_get_event() {
    encoder.tick(); // Call regularly if not using ISRs handled by the library
    long new_pos = encoder.getPosition();
    InputEvent event = InputEvent::NONE;

    if (new_pos > last_encoder_pos) {
        event = InputEvent::ENCODER_DOWN; // Or UP, depending on your wiring/interpretation
    } else if (new_pos < last_encoder_pos) {
        event = InputEvent::ENCODER_UP;
    }
    last_encoder_pos = new_pos;

    if (encoder_button_pressed) {
        event = InputEvent::ENCODER_PRESS;
        encoder_button_pressed = false; // Consume the event
    }
    return event;
}

// --- Simplified accessors for main loop if not using full LVGL integration for input yet ---
int8_t get_encoder_diff() {
    encoder.tick();
    long new_pos = encoder.getPosition();
    int diff = 0;
    if (new_pos != last_encoder_pos) {
        diff = new_pos - last_encoder_pos;
        encoder.setPosition(0); // Reset for delta next time, or manage absolute
        last_encoder_pos = 0; // If resetting above
    }
    return (int8_t)diff;
}

bool is_encoder_pressed() {
    bool pressed = encoder_button_pressed;
    if (pressed) {
        encoder_button_pressed = false; // Consume
    }
    return pressed;
}

void clear_encoder_state() {
    encoder_button_pressed = false;
    last_encoder_pos = encoder.getPosition();
}