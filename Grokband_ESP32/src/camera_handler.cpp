#include "camera_handler.h"
#include "config.h" // For CAM_PINS
#include "esp_log.h" // For ESP_LOGE, etc.

static const char* TAG = "camera";

bool camera_init() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD; // SIOD
    config.pin_sccb_scl = CAM_PIN_SIOC; // SIOC
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA; // (320x240) Start with this, then maybe smaller for TFLite
    config.pixel_format = PIXFORMAT_JPEG; // Or PIXFORMAT_RGB565 / PIXFORMAT_GRAYSCALE
                                          // For TFLite, you'll likely want GRAYSCALE or RGB
                                          // If JPEG, you'll need to decode it first.
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM if available
    config.jpeg_quality = 12; // 0-63, lower means higher quality
    config.fb_count = 1;      // If more than one, can do double buffering.

    // For TFLite, you might want a smaller frame size and non-JPEG format
    // e.g., FRAMESIZE_96X96 if your model takes 96x96 input
    // config.frame_size = FRAMESIZE_96X96;
    // config.pixel_format = PIXFORMAT_GRAYSCALE;


    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return false;
    }
    ESP_LOGI(TAG, "Camera Initialized");
    
    // Optional: Adjust sensor settings (exposure, gain, etc.)
    // sensor_t * s = esp_camera_sensor_get();
    // s->set_vflip(s, 1); // flip camera vertically

    return true;
}

camera_fb_t* camera_capture_frame() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera frame capture failed");
        return nullptr;
    }
    // ESP_LOGI(TAG, "Frame captured: %u bytes, %dx%d", fb->len, fb->width, fb->height);
    return fb;
}

void camera_return_frame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

// VERY BASIC PREPROCESSING EXAMPLE - YOU MUST ADAPT THIS
// Assumes input is GRAYSCALE and output is also GRAYSCALE for the model
// For RGB or other conversions, this will be more complex.
// img_transform from esp-dl can be very_helpful here if using their TFLite helper
// https://github.com/espressif/esp-dl/blob/master/include/image_util.hpp
bool preprocess_camera_frame(camera_fb_t* fb, uint8_t* model_input_buffer, int input_w, int input_h, int input_channels) {
    if (!fb || !fb->buf) {
        ESP_LOGE(TAG, "Invalid frame buffer for preprocessing");
        return false;
    }
    if (fb->format != PIXFORMAT_GRAYSCALE && input_channels == 1) {
         ESP_LOGE(TAG, "Frame format is not GRAYSCALE. Preprocessing needs to handle conversion.");
         // You might need to convert from RGB565 or JPEG to GRAYSCALE here.
         // For JPEG: fmt2rgb888, then rgb2gray
         // For RGB565: directly to gray (R*0.299 + G*0.587 + B*0.114)
         return false; // Placeholder - implement conversion
    }
    if (fb->format == PIXFORMAT_JPEG) {
        ESP_LOGE(TAG, "Frame format is JPEG. It needs to be decoded first for TFLite raw input.");
        // Use jpg2rgb565 or similar from esp_camera.h, then process
        return false; // Placeholder - implement JPEG decoding
    }


    // This is a naive resize (sub-sampling). For better quality, use bilinear interpolation.
    // This also assumes fb->width, fb->height >= input_w, input_h
    int H = fb->height;
    int W = fb->width;

    // Example: simple crop and resize (if needed, or just take center crop)
    // This is just a placeholder for actual image resizing/scaling logic
    // For a real application, you'd use a library function for proper resizing (e.g., from esp-dl)
    
    // Example for center crop if camera frame is larger than model input
    int y_offset = (H - input_h) / 2;
    int x_offset = (W - input_w) / 2;

    for (int y = 0; y < input_h; ++y) {
        for (int x = 0; x < input_w; ++x) {
            int fb_y = y_offset + y;
            int fb_x = x_offset + x;
            if (fb_y < H && fb_x < W) { // bounds check
                // Assuming single channel (grayscale)
                 if (input_channels == 1) {
                    model_input_buffer[y * input_w + x] = fb->buf[fb_y * W + fb_x];
                 } else if (input_channels == 3) {
                    // If model needs RGB, and camera provides RGB (e.g. RGB565 decoded to RGB888)
                    // This part highly depends on camera output format and model input format
                    // Example for RGB888 input (fb->buf points to RGB888 data)
                    // model_input_buffer[(y * input_w + x) * 3 + 0] = fb->buf[(fb_y * W + fb_x) * 3 + 0]; // R
                    // model_input_buffer[(y * input_w + x) * 3 + 1] = fb->buf[(fb_y * W + fb_x) * 3 + 1]; // G
                    // model_input_buffer[(y * input_w + x) * 3 + 2] = fb->buf[(fb_y * W + fb_x) * 3 + 2]; // B
                    ESP_LOGE(TAG, "RGB preprocessing not fully implemented in this skeleton.");
                    return false;
                 }
            } else {
                model_input_buffer[y * input_w + x] = 0; // Padding if needed
            }
        }
    }
    // Normalize if your model expects it (e.g., input range -1 to 1 or 0 to 1 float)
    // For uint8_t models, normalization might be part of the model (quantization).
    // If model expects float input:
    // for(int i=0; i < input_w * input_h * input_channels; ++i) {
    //    float_model_input_buffer[i] = (model_input_buffer[i] / 255.0f) - 0.5f; // Example: scale to -0.5 to 0.5
    // }

    return true;
}