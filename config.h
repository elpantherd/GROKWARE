Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.

#ifndef CONFIG_H
#define CONFIG_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Hardware Pin Definitions
#define CAMERA_SDA_PIN      GPIO_NUM_26
#define CAMERA_SCL_PIN      GPIO_NUM_27
#define OLED_SDA_PIN        GPIO_NUM_21
#define OLED_SCL_PIN        GPIO_NUM_22
#define ENCODER_PIN_A       GPIO_NUM_32
#define ENCODER_PIN_B       GPIO_NUM_33
#define ENCODER_BUTTON_PIN  GPIO_NUM_25
#define LED_PIN             GPIO_NUM_19
#define VIBRATION_PIN       GPIO_NUM_18

// Display Configuration
#define OLED_WIDTH          128
#define OLED_HEIGHT         128
#define NUM_QUICK_RESPONSES 8
#define DISPLAY_RADIUS      50
#define CENTER_X            (OLED_WIDTH / 2)
#define CENTER_Y            (OLED_HEIGHT / 2)

// Camera Configuration
#define CAMERA_WIDTH        320
#define CAMERA_HEIGHT       240

// MQTT Configuration
#define MQTT_BROKER_URI     "mqtt://broker.example.com"
#define MQTT_PORT           1883
#define TOPIC_SIGN_TEXT     "sign_language_text"
#define TOPIC_VOICE_TEXT    "voice_text"

// Shared Resources
extern QueueHandle_t image_queue;
extern QueueHandle_t text_queue;
extern SemaphoreHandle_t selection_mutex;
extern SemaphoreHandle_t message_received_sem;
extern int current_selection;
extern const char *quick_responses[NUM_QUICK_RESPONSES];

// Function Prototypes
void comm_send_text(const char *text);

#endif // CONFIG_H
