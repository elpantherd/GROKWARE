Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"
#include "camera_task.h"
#include "recognition_task.h"
#include "display_task.h"
#include "encoder_task.h"
#include "comm_task.h"
#include "feedback_task.h"

static const char *TAG = "main";

QueueHandle_t image_queue;
QueueHandle_t text_queue;
SemaphoreHandle_t selection_mutex;
SemaphoreHandle_t message_received_sem;
int current_selection = 0;
const char *quick_responses[NUM_QUICK_RESPONSES] = {
    "Yes", "No", "Hello", "Goodbye",
    "Thanks", "Help", "Okay", "Wait"
};

static void initialize_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erasing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
}

static void initialize_wifi(void) {
    // Placeholder for Wi-Fi initialization
    // In a real implementation, use ESP-IDF Wi-Fi APIs
    ESP_LOGI(TAG, "Wi-Fi initialized (placeholder)");
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Grokband application");

    // Initialize NVS
    initialize_nvs();

    // Initialize Wi-Fi
    initialize_wifi();

    // Create queues and semaphores
    image_queue = xQueueCreate(2, sizeof(uint8_t *));
    text_queue = xQueueCreate(5, sizeof(char *));
    selection_mutex = xSemaphoreCreateMutex();
    message_received_sem = xSemaphoreCreateBinary();

    if (!image_queue || !text_queue || !selection_mutex || !message_received_sem) {
        ESP_LOGE(TAG, "Failed to create RTOS resources");
        return;
    }

    // Create tasks
    xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, NULL);
    xTaskCreate(recognition_task, "recognition_task", 8192, NULL, 5, NULL);
    xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
    xTaskCreate(encoder_task, "encoder_task", 2048, NULL, 5, NULL);
    xTaskCreate(comm_task, "comm_task", 4096, NULL, 5, NULL);
    xTaskCreate(feedback_task, "feedback_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "All tasks created successfully");

    // Main loop (optional monitoring)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "System running...");
    }
}
