#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
// Placeholder for Arducam library
#include "arducam.h"

static const char *TAG = "camera_task";

static void configure_camera(void) {
    // Configure OV2640 camera settings
    arducam_config_t config = {
        .sda_pin = CAMERA_SDA_PIN,
        .scl_pin = CAMERA_SCL_PIN,
        .width = CAMERA_WIDTH,
        .height = CAMERA_HEIGHT,
        .format = PIXFORMAT_GRAYSCALE // Assuming model uses grayscale
    };
    arducam_init(&config);
    ESP_LOGI(TAG, "Camera configured");
}

void camera_task(void *pvParameters) {
    ESP_LOGI(TAG, "Camera task started");

    // Initialize camera
    configure_camera();

    while (1) {
        // Capture image
        uint8_t *image = arducam_capture();
        if (!image) {
            ESP_LOGE(TAG, "Failed to capture image");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Send image to recognition task
        if (xQueueSend(image_queue, &image, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Image queue full, dropping frame");
            free(image);
        }

        // Control frame rate (e.g., 10 FPS)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
