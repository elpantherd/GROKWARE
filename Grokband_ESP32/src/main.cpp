#include <Arduino.h>
#include "config.h"
#include "mqtt_handler.h"
#include "display_handler.h"
#include "input_handler.h"
#include "camera_handler.h"
#include "sign_language_model.h"
#include "notifications.h"

// For TFLite model input buffer
uint8_t model_input_buf[TFLITE_MODEL_INPUT_HEIGHT * TFLITE_MODEL_INPUT_WIDTH * TFLITE_MODEL_INPUT_CHANNELS];

// State machine or application mode
enum class AppMode {
    IDLE,
    SHOWING_MESSAGE,
    QUICK_RESPONSE_NAV,
    SIGNING // Capturing and processing sign language
};
AppMode current_mode = AppMode::IDLE;

int selected_quick_response_idx = 0;
unsigned long last_activity_time = 0;
const unsigned long MESSAGE_DISPLAY_TIMEOUT_MS = 10000; // 10 seconds
const unsigned long SIGNING_TIMEOUT_MS = 15000; // 15 seconds in signing mode


// MQTT Callback function
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    char msg_buffer[128]; // Be careful with buffer overflows
    int msg_len = min((unsigned int)sizeof(msg_buffer) - 1, length);
    strncpy(msg_buffer, (char*)payload, msg_len);
    msg_buffer[msg_len] = '\0';
    Serial.println(msg_buffer);

    if (strcmp(topic, MQTT_TOPIC_SPEECH_TO_SIGN) == 0) {
        display_show_message(msg_buffer);
        notification_alert_message();
        current_mode = AppMode::SHOWING_MESSAGE;
        last_activity_time = millis();
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for serial connection (for debugging)
    Serial.println("Grokband Starting...");

    notification_init();
    input_init(); // Before display if display uses input for LVGL
    display_init(); // LVGL init
    
    // Example of how LVGL input might be set up (if using its indev system)
    // static lv_indev_drv_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    // indev_drv.type = LV_INDEV_TYPE_ENCODER;
    // indev_drv.read_cb = lvgl_encoder_read_fake; // from input_handler.cpp
    // lv_indev_t *encoder_indev = lv_indev_drv_register(&indev_drv);
    // You also need to associate this indev with a group and objects if objects are to be focused/clicked by encoder.

    display_show_status("Connecting WiFi...");
    setup_wifi();
    display_show_status("Connecting MQTT...");
    setup_mqtt(mqtt_callback);

    display_show_status("Initializing Camera...");
    if (!camera_init()) {
        display_show_message("Camera Init Failed!");
        // Maybe enter a fault state
        while(1) delay(1000);
    }

    display_show_status("Initializing TFLite...");
    if (!tflite_init()) {
        display_show_message("TFLite Init Failed!");
        // Maybe enter a fault state
        while(1) delay(1000);
    }
    
    display_show_message("Grokband Ready");
    display_show_status(is_mqtt_connected() ? "MQTT Connected" : "MQTT Disconnected");
    current_mode = AppMode::IDLE;
    last_activity_time = millis();

    // Quick test for notifications
    // notification_alert_message(true); 
    // delay(1000);
}


void loop() {
    mqtt_loop(); // Keep MQTT connection alive and process incoming
    display_update_loop(); // Keep LVGL refreshing

    // Non-LVGL direct input handling (simpler for start)
    int8_t encoder_change = get_encoder_diff(); // From input_handler
    bool encoder_pressed = is_encoder_pressed(); // From input_handler

    // Timeout to return to IDLE from message display or quick response
    if ((current_mode == AppMode::SHOWING_MESSAGE || current_mode == AppMode::QUICK_RESPONSE_NAV) &&
        (millis() - last_activity_time > MESSAGE_DISPLAY_TIMEOUT_MS)) {
        display_clear();
        display_show_message("Grokband Ready"); // Or previous status
        current_mode = AppMode::IDLE;
    }
     if (current_mode == AppMode::SIGNING && (millis() - last_activity_time > SIGNING_TIMEOUT_MS)) {
        display_show_message("Signing timed out.");
        current_mode = AppMode::IDLE;
    }


    switch (current_mode) {
        case AppMode::IDLE:
            if (encoder_pressed) {
                // Enter quick response mode or signing mode
                // Let's say short press = quick response, long press = signing
                // For now, any press goes to quick response
                current_mode = AppMode::QUICK_RESPONSE_NAV;
                selected_quick_response_idx = 0;
                display_show_quick_responses_ui(quick_responses, NUM_QUICK_RESPONSES, selected_quick_response_idx);
                last_activity_time = millis();
                Serial.println("Mode: QUICK_RESPONSE_NAV");
            }
            // Potentially add a way to enter SIGNING mode (e.g., double click, or if no quick responses selected)
            // Or a dedicated "start signing" UI option in quick responses.
            break;

        case AppMode::SHOWING_MESSAGE:
            if (encoder_pressed) { // Acknowledge message / go to quick reply
                current_mode = AppMode::QUICK_RESPONSE_NAV;
                selected_quick_response_idx = 0;
                display_show_quick_responses_ui(quick_responses, NUM_QUICK_RESPONSES, selected_quick_response_idx);
                last_activity_time = millis();
                Serial.println("Mode: QUICK_RESPONSE_NAV from SHOWING_MESSAGE");
            }
            break;

        case AppMode::QUICK_RESPONSE_NAV:
            if (encoder_change != 0) {
                selected_quick_response_idx += encoder_change;
                if (selected_quick_response_idx < 0) selected_quick_response_idx = NUM_QUICK_RESPONSES - 1;
                if (selected_quick_response_idx >= NUM_QUICK_RESPONSES) selected_quick_response_idx = 0;
                display_show_quick_responses_ui(quick_responses, NUM_QUICK_RESPONSES, selected_quick_response_idx);
                last_activity_time = millis();
            }
            if (encoder_pressed) {
                const char* resp = quick_responses[selected_quick_response_idx];
                mqtt_publish(MQTT_TOPIC_QUICK_RESPONSE, resp);
                display_show_message("Sent Quick Resp.");
                current_mode = AppMode::SHOWING_MESSAGE; // Show confirmation
                last_activity_time = millis();
                Serial.print("Sent Quick Response: "); Serial.println(resp);
            }
            break;

        case AppMode::SIGNING:
            // This is the core loop for sign language recognition
            // Should be triggered by user action (e.g. from menu or long press)
            display_show_message("Signing...");
            camera_fb_t* fb = camera_capture_frame();
            if (fb) {
                // Preprocess the frame
                // Ensure your camera is set to a format preprocess_camera_frame can handle,
                // or that preprocess_camera_frame does the necessary conversions (e.g. JPEG decode, RGB2GRAY)
                if (preprocess_camera_frame(fb, model_input_buf, TFLITE_MODEL_INPUT_WIDTH, TFLITE_MODEL_INPUT_HEIGHT, TFLITE_MODEL_INPUT_CHANNELS)) {
                    // Run inference
                    float scores[TFLITE_NUM_CLASSES];
                    int detected_class_idx = tflite_predict(model_input_buf, scores);
                    
                    if (detected_class_idx != -1) {
                        const char* sign_label = get_class_label(detected_class_idx);
                        char msg_to_send[64];
                        snprintf(msg_to_send, sizeof(msg_to_send), "%s (%.2f)", sign_label, scores[detected_class_idx]);
                        
                        display_show_message(msg_to_send); // Show what was detected
                        mqtt_publish(MQTT_TOPIC_SIGN_TO_TEXT, sign_label); // Send just the label
                        Serial.print("Detected Sign: "); Serial.println(sign_label);
                        
                        // Potentially: Add logic to accumulate signs for a sentence before sending
                        // Or exit signing mode after one sign
                        current_mode = AppMode::IDLE; // Exit signing mode after one detection for now
                        display_show_message("Sent Sign."); // Confirmation
                        last_activity_time = millis();

                    } else {
                        // Serial.println("No sign detected or low confidence.");
                        // Keep trying or show "no detection" on display
                    }
                } else {
                    Serial.println("Frame preprocessing failed.");
                    display_show_message("Preprocessing err");
                    current_mode = AppMode::IDLE;
                }
                camera_return_frame(fb); // IMPORTANT: return frame buffer
            } else {
                Serial.println("Failed to capture frame.");
                display_show_message("Cam Capture Err");
                current_mode = AppMode::IDLE;
            }
            // For continuous signing, remove return to IDLE until user explicitly exits
            // last_activity_time = millis(); // Keep resetting timeout if in continuous mode
            break;
    }

    // One way to enter signing mode (e.g. from IDLE, with a longer press or specific UI choice)
    // This logic is currently missing, you'd need to differentiate encoder presses
    // or add a "Start Signing" option to the quick response menu.
    // For now, let's assume if IDLE and a rotary click happens, it goes to QUICK_RESPONSE_NAV.
    // If another click happens in QUICK_RESPONSE_NAV on a special "Start Sign" item, then mode becomes SIGNING.
    // Or, as a HACK for testing, if encoder is pressed AND HELD for >1 second while IDLE, go to SIGNING.
    // This requires more advanced input handling than currently implemented.
    // For now, you can manually change current_mode to AppMode::SIGNING in setup for testing:
    // if(setup_is_done) current_mode = AppMode::SIGNING; 

    delay(10); // Small delay to prevent busy-looping too fast
}