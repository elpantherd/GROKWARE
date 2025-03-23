Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "feedback_task";

static void configure_feedback(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN) | (1ULL << VIBRATION_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0);
    gpio_set_level(VIBRATION_PIN, 0);
    ESP_LOGI(TAG, "Feedback GPIO configured");
}

void feedback_task(void *pvParameters) {
    ESP_LOGI(TAG, "Feedback task started");

    configure_feedback();

    while (1) {
        if (xSemaphoreTake(message_received_sem, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Activating feedback");
            gpio_set_level(LED_PIN, 1);
            gpio_set_level(VIBRATION_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(LED_PIN, 0);
            gpio_set_level(VIBRATION_PIN, 0);
        }
    }
}
