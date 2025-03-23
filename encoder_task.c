Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "encoder_task";

static int last_state = 0;

static void configure_encoder(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_PIN_A) | (1ULL << ENCODER_PIN_B) | (1ULL << ENCODER_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Encoder GPIO configured");
}

void encoder_task(void *pvParameters) {
    ESP_LOGI(TAG, "Encoder task started");

    configure_encoder();

    while (1) {
        int state_a = gpio_get_level(ENCODER_PIN_A);
        int state_b = gpio_get_level(ENCODER_PIN_B);

        if (state_a != last_state) {
            if (state_b != state_a) {
                // Clockwise
                xSemaphoreTake(selection_mutex, portMAX_DELAY);
                current_selection = (current_selection + 1) % NUM_QUICK_RESPONSES;
                xSemaphoreGive(selection_mutex);
                ESP_LOGI(TAG, "Encoder turned clockwise, selection: %d", current_selection);
            } else {
                // Counterclockwise
                xSemaphoreTake(selection_mutex, portMAX_DELAY);
                current_selection = (current_selection - 1 + NUM_QUICK_RESPONSES) % NUM_QUICK_RESPONSES;
                xSemaphoreGive(selection_mutex);
                ESP_LOGI(TAG, "Encoder turned counterclockwise, selection: %d", current_selection);
            }
            last_state = state_a;
        }

        if (gpio_get_level(ENCODER_BUTTON_PIN) == 0) {
            xSemaphoreTake(selection_mutex, portMAX_DELAY);
            int sel = current_selection;
            xSemaphoreGive(selection_mutex);
            comm_send_text(quick_responses[sel]);
            ESP_LOGI(TAG, "Button pressed, sent: %s", quick_responses[sel]);
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
