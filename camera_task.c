Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.

// camera_task.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

// Placeholder for Arducam library
#include "arducam.h"

#define CAMERA_SDA_PIN GPIO_NUM_21
#define CAMERA_SCL_PIN GPIO_NUM_22
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240
#define CAMERA_RESET_PIN GPIO_NUM_23

static const char *TAG = "camera_task";
extern QueueHandle_t image_queue;

typedef struct {
    int width;
    int height;
    int format;
    int frame_rate;
    bool auto_exposure;
    bool auto_white_balance;
    int brightness;
    int contrast;
} camera_settings_t;

// Default camera settings
static camera_settings_t cam_settings = {
    .width = CAMERA_WIDTH,
    .height = CAMERA_HEIGHT,
    .format = PIXFORMAT_GRAYSCALE,
    .frame_rate = 10,
    .auto_exposure = true,
    .auto_white_balance = true,
    .brightness = 0,
    .contrast = 0
};

// Reset camera hardware
static esp_err_t camera_hardware_reset(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CAMERA_RESET_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(CAMERA_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CAMERA_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "Camera hardware reset completed");
    return ESP_OK;
}

// Initialize and configure camera
static esp_err_t configure_camera(void) {
    arducam_config_t config = {
        .sda_pin = CAMERA_SDA_PIN,
        .scl_pin = CAMERA_SCL_PIN,
        .width = cam_settings.width,
        .height = cam_settings.height,
        .format = cam_settings.format,
        .frame_rate = cam_settings.frame_rate,
        .auto_exposure = cam_settings.auto_exposure,
        .auto_white_balance = cam_settings.auto_white_balance
    };

    esp_err_t ret = arducam_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = arducam_set_brightness(cam_settings.brightness);
    if (ret != ESP_OK) ESP_LOGW(TAG, "Failed to set brightness: %s", esp_err_to_name(ret));

    ret = arducam_set_contrast(cam_settings.contrast);
    if (ret != ESP_OK) ESP_LOGW(TAG, "Failed to set contrast: %s", esp_err_to_name(ret));

    // Additional configurations
    ret = arducam_set_exposure_time(500); // Example value
    if (ret != ESP_OK) ESP_LOGW(TAG, "Failed to set exposure time");

    ret = arducam_set_gain(100); // Example value
    if (ret != ESP_OK) ESP_LOGW(TAG, "Failed to set gain");

    ESP_LOGI(TAG, "Camera fully configured");
    return ESP_OK;
}

// Camera diagnostic function
static void camera_diagnostics(void) {
    int firmware_version = arducam_get_firmware_version();
    ESP_LOGI(TAG, "Camera firmware version: %d", firmware_version);
    int status = arducam_get_status();
    ESP_LOGI(TAG, "Camera status code: %d", status);
}

void camera_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting camera task");
    camera_hardware_reset();

    int init_attempts = 0;
    const int max_attempts = 3;
    while (init_attempts < max_attempts) {
        if (configure_camera() == ESP_OK) {
            break;
        }
        ESP_LOGE(TAG, "Camera init attempt %d failed", init_attempts + 1);
        init_attempts++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (init_attempts >= max_attempts) {
        ESP_LOGE(TAG, "Camera initialization failed after %d attempts", max_attempts);
        vTaskDelete(NULL);
    }

    camera_diagnostics();

    while (1) {
        uint8_t *image = arducam_capture();
        if (!image) {
            ESP_LOGE(TAG, "Image capture failed");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ESP_LOGD(TAG, "Captured image, size: %d bytes", CAMERA_WIDTH * CAMERA_HEIGHT);
        if (xQueueSend(image_queue, &image, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Image queue full, dropping frame");
            free(image);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Target 10 FPS
    }
}
