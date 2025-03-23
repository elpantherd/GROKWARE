Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
// utils.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

static const char *TAG = "utils";

// Log system status
void log_system_status(void) {
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();
    uint32_t uptime = esp_timer_get_time() / 1000000; // Seconds
    ESP_LOGI(TAG, "System Status:");
    ESP_LOGI(TAG, "  Free Heap: %d bytes", free_heap);
    ESP_LOGI(TAG, "  Min Free Heap: %d bytes", min_free_heap);
    ESP_LOGI(TAG, "  Uptime: %d seconds", uptime);
}

// Log task information
void log_task_info(void) {
    TaskStatus_t *task_array = NULL;
    UBaseType_t array_size = uxTaskGetNumberOfTasks();
    UBaseType_t tasks_reported;

    task_array = (TaskStatus_t*)malloc(array_size * sizeof(TaskStatus_t));
    if (!task_array) {
        ESP_LOGE(TAG, "Failed to allocate memory for task info");
        return;
    }

    tasks_reported = uxTaskGetSystemState(task_array, array_size, NULL);
    ESP_LOGI(TAG, "Task Information (%d tasks):", tasks_reported);
    for (UBaseType_t i = 0; i < tasks_reported; i++) {
        ESP_LOGI(TAG, "  Task: %s, State: %d, Priority: %d, Stack High Water Mark: %d",
                 task_array[i].pcTaskName,
                 task_array[i].eCurrentState,
                 task_array[i].uxCurrentPriority,
                 task_array[i].usStackHighWaterMark);
    }
    free(task_array);
}

// Save log to NVS
esp_err_t save_log_to_nvs(const char *key, const char *message) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("log_storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, key, message);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write log to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Log saved to NVS under key: %s", key);
    }

    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}

// Read log from NVS
esp_err_t read_log_from_nvs(const char *key, char *buffer, size_t *buf_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("log_storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for reading: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvs_handle, key, buffer, buf_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read log from NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Log read from NVS: %s", buffer);
    }

    nvs_close(nvs_handle);
    return err;
}

// Utility to restart system with reason
void system_restart(const char *reason) {
    ESP_LOGW(TAG, "Restarting system: %s", reason);
    save_log_to_nvs("last_restart", reason);
    esp_restart();
}

// Memory usage monitor task
void memory_monitor_task(void *pvParameters) {
    while (1) {
        log_system_status();
        log_task_info();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Every minute
    }
}

// Initialize utilities
void utils_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash");
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "Utilities initialized");

    // Start memory monitor task
    xTaskCreate(memory_monitor_task, "memory_monitor", 4096, NULL, 2, NULL);
}
