Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
// wifi_setup.c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define WIFI_SSID_DEFAULT "your_ssid"
#define WIFI_PASS_DEFAULT "your_password"
#define MAX_RETRY 5
#define WIFI_TIMEOUT_MS 30000
#define WIFI_SCAN_MAX_AP 10

static const char *TAG = "wifi_setup";
static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;
static bool wifi_connected = false;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_SCAN_DONE_BIT BIT2

// Wi-Fi credentials structure
typedef struct {
    char ssid[32];
    char password[64];
    wifi_auth_mode_t auth_mode;
} wifi_credentials_t;

static wifi_credentials_t wifi_creds = {
    .ssid = WIFI_SSID_DEFAULT,
    .password = WIFI_PASS_DEFAULT,
    .auth_mode = WIFI_AUTH_WPA2_PSK
};

// Event handler for Wi-Fi and IP events
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi STA started, attempting connection");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifi_connected = false;
                if (retry_count < MAX_RETRY) {
                    ESP_LOGI(TAG, "Wi-Fi disconnected, retrying (%d/%d)", retry_count + 1, MAX_RETRY);
                    esp_wifi_connect();
                    retry_count++;
                } else {
                    ESP_LOGE(TAG, "Max retries reached, setting fail bit");
                    xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                }
                break;
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Wi-Fi scan completed");
                xEventGroupSetBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
                break;
            default:
                ESP_LOGW(TAG, "Unhandled Wi-Fi event: %ld", event_id);
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: %s", ip4addr_ntoa(&event->ip_info.ip));
        retry_count = 0;
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Scan for available Wi-Fi networks
static void wifi_scan(void) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 120,
        .scan_time.active.max = 150
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    uint16_t ap_count = WIFI_SCAN_MAX_AP;
    wifi_ap_record_t ap_records[WIFI_SCAN_MAX_AP];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
    ESP_LOGI(TAG, "Found %d access points:", ap_count);
    for (int i = 0; i < ap_count; i++) {
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_records[i].ssid, ap_records[i].rssi);
    }
}

// Load Wi-Fi credentials from NVS
static esp_err_t load_wifi_credentials(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        size_t ssid_len = sizeof(wifi_creds.ssid);
        size_t pass_len = sizeof(wifi_creds.password);
        err = nvs_get_str(nvs_handle, "ssid", wifi_creds.ssid, &ssid_len);
        if (err != ESP_OK) ESP_LOGW(TAG, "SSID not found in NVS");
        err = nvs_get_str(nvs_handle, "password", wifi_creds.password, &pass_len);
        if (err != ESP_OK) ESP_LOGW(TAG, "Password not found in NVS");
        nvs_close(nvs_handle);
    }
    return err;
}

// Initialize and connect Wi-Fi in STA mode
void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    // Load credentials from NVS if available
    if (load_wifi_credentials() != ESP_OK) {
        ESP_LOGI(TAG, "Using default Wi-Fi credentials");
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = wifi_creds.auth_mode,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char*)wifi_config.sta.ssid, wifi_creds.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, wifi_creds.password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi STA mode initialized, waiting for connection...");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, WIFI_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Successfully connected to AP SSID: %s", wifi_creds.ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP after %d retries", MAX_RETRY);
        wifi_scan(); // Scan for alternatives on failure
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection timed out");
    }

    // Cleanup event handlers
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
}
