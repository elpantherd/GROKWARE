Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
#include "esp_mqtt_client.h"

static const char *TAG = "comm_task";
static esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(client, TOPIC_VOICE_TEXT, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed to %s", TOPIC_VOICE_TEXT);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received message on %s", event->topic);
            xSemaphoreGive(message_received_sem);
            break;
        default:
            break;
    }
}

void comm_send_text(const char *text) {
    if (client) {
        int msg_id = esp_mqtt_client_publish(client, TOPIC_SIGN_TEXT, text, 0, 0, 0);
        ESP_LOGI(TAG, "Sent '%s' to %s, msg_id=%d", text, TOPIC_SIGN_TEXT, msg_id);
    }
}

void comm_task(void *pvParameters) {
    ESP_LOGI(TAG, "Communication task started");

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URI,
        .port = MQTT_PORT,
        // Add TLS configuration if needed
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "MQTT client running...");
    }
}
