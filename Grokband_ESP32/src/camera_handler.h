#ifndef CAMERA_HANDLER_H
#define CAMERA_HANDLER_H

#include "esp_camera.h"

bool camera_init();
camera_fb_t* camera_capture_frame();
void camera_return_frame(camera_fb_t* fb);

// For TFLite:
// You'll need a function to preprocess the frame (resize, grayscale, normalize)
// This depends heavily on your TFLite model's input requirements
bool preprocess_camera_frame(camera_fb_t* fb, uint8_t* model_input_buffer, int input_w, int input_h, int input_channels);

#endif // CAMERA_HANDLER_H