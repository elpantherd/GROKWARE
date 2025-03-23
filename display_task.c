Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
#include "lvgl.h"
// Placeholder for display driver
#include "oled_driver.h"

static const char *TAG = "display_task";

static lv_obj_t *labels[NUM_QUICK_RESPONSES];
static lv_style_t style_normal, style_highlight;

static void initialize_styles(void) {
    lv_style_init(&style_normal);
    lv_style_set_text_color(&style_normal, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_color(&style_normal, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_style_init(&style_highlight);
    lv_style_set_text_color(&style_highlight, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_bg_color(&style_highlight, LV_STATE_DEFAULT, LV_COLOR_WHITE);
}

static void create_ui(void) {
    for (int i = 0; i < NUM_QUICK_RESPONSES; i++) {
        float angle = (i * 360.0 / NUM_QUICK_RESPONSES) * (M_PI / 180.0);
        int x = CENTER_X + DISPLAY_RADIUS * cos(angle);
        int y = CENTER_Y + DISPLAY_RADIUS * sin(angle);
        labels[i] = lv_label_create(lv_scr_act(), NULL);
        lv_label_set_text(labels[i], quick_responses[i]);
        lv_obj_align(labels[i], NULL, LV_ALIGN_CENTER, x - CENTER_X, y - CENTER_Y);
        lv_obj_add_style(labels[i], LV_LABEL_PART_MAIN, &style_normal);
    }
    ESP_LOGI(TAG, "Circular UI created");
}

static void update_selection(int index) {
    for (int i = 0; i < NUM_QUICK_RESPONSES; i++) {
        lv_obj_clean_style_list(labels[i], LV_LABEL_PART_MAIN);
        if (i == index) {
            lv_obj_add_style(labels[i], LV_LABEL_PART_MAIN, &style_highlight);
        } else {
            lv_obj_add_style(labels[i], LV_LABEL_PART_MAIN, &style_normal);
        }
    }
}

void display_task(void *pvParameters) {
    ESP_LOGI(TAG, "Display task started");

    // Initialize LVGL and display
    lv_init();
    oled_driver_init(OLED_SDA_PIN, OLED_SCL_PIN);

    // Initialize styles and UI
    initialize_styles();
    create_ui();

    int last_selection = -1;

    while (1) {
        xSemaphoreTake(selection_mutex, portMAX_DELAY);
        int sel = current_selection;
        xSemaphoreGive(selection_mutex);

        if (sel != last_selection) {
            update_selection(sel);
            last_selection = sel;
            ESP_LOGI(TAG, "Selection updated to %d", sel);
        }

        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
