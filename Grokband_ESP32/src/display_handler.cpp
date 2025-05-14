#include "display_handler.h"
#include "config.h" // For display pins if not passed directly
#include <TFT_eSPI.h> // Or your specific display library

// For LVGL:
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[LV_HOR_RES_MAX * 10]; // Adjust buffer size
TFT_eSPI tft = TFT_eSPI(); // Your display object

lv_obj_t *main_screen;
lv_obj_t *message_label;
lv_obj_t *status_label;
lv_obj_t *quick_response_list; // For LVGL list or similar widget

// LVGL display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// LVGL input device (rotary encoder) read callback
// This needs to be integrated with your input_handler.cpp
// For simplicity, we'll assume input_handler.cpp calls functions here directly for now.
// Or, input_handler calls a registered callback from display_handler.

void display_init() {
    tft.begin();
    tft.setRotation(0); // Adjust as needed

    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width(); // Adjust to your OLED width
    disp_drv.ver_res = tft.height(); // Adjust to your OLED height
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // TODO: Initialize input device driver for LVGL (rotary encoder)
    // static lv_indev_drv_t indev_drv;
    // lv_indev_drv_init(&indev_drv);
    // indev_drv.type = LV_INDEV_TYPE_ENCODER;
    // indev_drv.read_cb = my_encoder_read_cb; // You need to implement this
    // lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv);


    // Create a simple UI
    main_screen = lv_scr_act();
    lv_obj_set_style_bg_color(main_screen, lv_color_black(), LV_PART_MAIN);

    message_label = lv_label_create(main_screen);
    lv_label_set_text(message_label, "Grokband Ready");
    lv_obj_set_style_text_color(message_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(message_label, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(message_label, tft.width() - 20);

    status_label = lv_label_create(main_screen);
    lv_label_set_text(status_label, "Status: ---");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    Serial.println("LVGL Display Initialized");
}

void display_update_loop() {
    lv_timer_handler(); // handles LVGL tasks
    delay(5);
}

void display_show_message(const char* message) {
    lv_label_set_text(message_label, message);
    Serial.print("Display: "); Serial.println(message);
}

void display_show_quick_responses_ui(const char* responses[], int count, int selected_idx) {
    // This is a placeholder. You'd typically use an lv_list or similar
    // For simplicity, we'll just update the message_label for now.
    char buffer[128];
    strcpy(buffer, "Quick Reply:\n");
    for (int i = 0; i < count; ++i) {
        if (i == selected_idx) strcat(buffer, "> ");
        strcat(buffer, responses[i]);
        strcat(buffer, "\n");
    }
    lv_label_set_text(message_label, buffer);
}

void display_clear() {
    lv_label_set_text(message_label, "");
}

void display_show_status(const char* status) {
    lv_label_set_text(status_label, status);
}