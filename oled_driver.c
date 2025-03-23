Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
// oled_driver.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "lvgl.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

static const char *TAG = "oled_driver";
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[OLED_WIDTH * OLED_HEIGHT / 10];

// Initialize I2C master
static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(err));
        return err;
    }
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
    }
    return err;
}

// Write command to OLED
static esp_err_t oled_write_cmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDRESS, data, 2, 1000 / portTICK_PERIOD_MS);
}

// Write data to OLED
static esp_err_t oled_write_data(uint8_t *data, size_t len) {
    uint8_t *buffer = malloc(len + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer for OLED data");
        return ESP_ERR_NO_MEM;
    }
    buffer[0] = 0x40; // Data mode
    memcpy(buffer + 1, data, len);
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDRESS, buffer, len + 1, 1000 / portTICK_PERIOD_MS);
    free(buffer);
    return ret;
}

// Initialize OLED hardware
static void oled_hardware_init(void) {
    oled_write_cmd(0xAE); // Display off
    oled_write_cmd(0xD5); // Set display clock
    oled_write_cmd(0x80);
    oled_write_cmd(0xA8); // Set multiplex ratio
    oled_write_cmd(OLED_HEIGHT - 1);
    oled_write_cmd(0xD3); // Set display offset
    oled_write_cmd(0x00);
    oled_write_cmd(0x40); // Set start line
    oled_write_cmd(0x8D); // Charge pump
    oled_write_cmd(0x14);
    oled_write_cmd(0x20); // Memory mode
    oled_write_cmd(0x00); // Horizontal addressing
    oled_write_cmd(0xA1); // Segment remap
    oled_write_cmd(0xC8); // COM output scan direction
    oled_write_cmd(0xDA); // COM pins
    oled_write_cmd(0x12);
    oled_write_cmd(0x81); // Contrast control
    oled_write_cmd(0xCF);
    oled_write_cmd(0xD9); // Pre-charge period
    oled_write_cmd(0xF1);
    oled_write_cmd(0xDB); // VCOMH deselect level
    oled_write_cmd(0x40);
    oled_write_cmd(0xA4); // Entire display on
    oled_write_cmd(0xA6); // Normal display
    oled_write_cmd(0xAF); // Display on
    ESP_LOGI(TAG, "OLED hardware initialized");
}

// Flush display buffer to OLED
static void disp_driver_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    int32_t width = area->x2 - area->x1 + 1;
    int32_t height = area->y2 - area->y1 + 1;
    uint8_t *data = (uint8_t*)malloc(width * height / 8);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate display buffer");
        lv_disp_flush_ready(drv);
        return;
    }

    // Convert LVGL colors to monochrome (simplified)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            int byte_idx = idx / 8;
            int bit_idx = idx % 8;
            if (lv_color_to1(color_p[idx])) {
                data[byte_idx] |= (1 << bit_idx);
            } else {
                data[byte_idx] &= ~(1 << bit_idx);
            }
        }
    }

    oled_write_cmd(0x21); // Set column address
    oled_write_cmd(area->x1);
    oled_write_cmd(area->x2);
    oled_write_cmd(0x22); // Set page address
    oled_write_cmd(area->y1 / 8);
    oled_write_cmd(area->y2 / 8);
    oled_write_data(data, width * height / 8);

    free(data);
    lv_disp_flush_ready(drv);
    ESP_LOGD(TAG, "Display flushed: %ldx%ld", width, height);
}

void oled_driver_init(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    oled_hardware_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, OLED_WIDTH * OLED_HEIGHT / 10);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = OLED_WIDTH;
    disp_drv.ver_res = OLED_HEIGHT;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    ESP_LOGI(TAG, "LVGL OLED driver initialized");
}
