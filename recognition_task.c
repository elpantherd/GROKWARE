Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
// recognition_task.c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Placeholder for model data
extern const unsigned char model_data[];
extern const char *labels[];
extern const int num_labels;

static const char *TAG = "recognition_task";
static tflite::MicroErrorReporter error_reporter;
static tflite::ErrorReporter* reporter = &error_reporter;
static const tflite::Model* model;
static tflite::MicroInterpreter* interpreter;
static TfLiteTensor* input_tensor;
static TfLiteTensor* output_tensor;
constexpr int kTensorArenaSize = 120 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];
extern QueueHandle_t image_queue;

typedef struct {
    int target_width;
    int target_height;
    float min_val;
    float max_val;
} preprocessing_config_t;

static preprocessing_config_t preprocess_config = {
    .target_width = 28,
    .target_height = 28,
    .min_val = 0.0f,
    .max_val = 1.0f
};

// Validate model integrity
static bool validate_model(void) {
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        reporter->Report("Model schema version %d not supported, expected %d", model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    const flatbuffers::Vector<flatbuffers::Offset<tflite::SubGraph>> *subgraphs = model->subgraphs();
    if (!subgraphs || subgraphs->size() == 0) {
        reporter->Report("No subgraphs found in model");
        return false;
    }
    ESP_LOGI(TAG, "Model validated: %d subgraphs", subgraphs->size());
    return true;
}

// Initialize TensorFlow Lite model
static void initialize_model(void) {
    model = tflite::GetModel(model_data);
    if (!validate_model()) {
        ESP_LOGE(TAG, "Model validation failed");
        return;
    }

    static tflite::MicroOpResolver<12> resolver;
    resolver.AddConv2D();
    resolver.AddMaxPool2D();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();
    resolver.AddRelu();
    resolver.AddDepthwiseConv2D();
    resolver.AddAveragePool2D();
    // Additional ops for flexibility
    resolver.AddPad();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddConcatenation();

    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        reporter->Report("Tensor allocation failed");
        ESP_LOGE(TAG, "TensorFlow Lite tensor allocation failed");
        return;
    }

    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);
    ESP_LOGI(TAG, "Input tensor: %d elements, Output tensor: %d elements", input_tensor->bytes / sizeof(float), output_tensor->bytes / sizeof(float));
    ESP_LOGI(TAG, "TensorFlow Lite model initialized");
}

// Preprocess image for model input
static void preprocess_image(uint8_t *image, float *input_data, int src_width, int src_height) {
    // Simple resize and normalization (placeholder for real implementation)
    int target_size = preprocess_config.target_width * preprocess_config.target_height;
    for (int i = 0; i < target_size; i++) {
        int src_x = (i % preprocess_config.target_width) * src_width / preprocess_config.target_width;
        int src_y = (i / preprocess_config.target_width) * src_height / preprocess_config.target_height;
        int src_idx = src_y * src_width + src_x;
        float value = (float)image[src_idx] / 255.0f;
        input_data[i] = (value - preprocess_config.min_val) / (preprocess_config.max_val - preprocess_config.min_val);
    }
    ESP_LOGD(TAG, "Image preprocessed to %dx%d", preprocess_config.target_width, preprocess_config.target_height);
}

// Postprocess inference output
static void postprocess_output(float *output_data, char *result, size_t result_size) {
    float max_score = output_data[0];
    int max_index = 0;
    for (int i = 1; i < num_labels; i++) {
        if (output_data[i] > max_score) {
            max_score = output_data[i];
            max_index = i;
        }
    }
    snprintf(result, result_size, "%s (%.2f)", labels[max_index], max_score);
    ESP_LOGI(TAG, "Inference result: %s", result);
}

void recognition_task(void *pvParameters) {
    initialize_model();
    if (!interpreter) {
        ESP_LOGE(TAG, "Model initialization failed, exiting task");
        vTaskDelete(NULL);
    }

    char result[64];
    while (1) {
        uint8_t *image;
        if (xQueueReceive(image_queue, &image, portMAX_DELAY) == pdTRUE) {
            preprocess_image(image, input_tensor->data.f, CAMERA_WIDTH, CAMERA_HEIGHT);
            TfLiteStatus invoke_status = interpreter->Invoke();
            if (invoke_status != kTfLiteOk) {
                ESP_LOGE(TAG, "Inference failed: %d", invoke_status);
                free(image);
                continue;
            }

            postprocess_output(output_tensor->data.f, result, sizeof(result));
            if (output_tensor->data.f[max_index] > 0.8) {
                // Placeholder for comm_send_text
                ESP_LOGI(TAG, "Sending recognized text: %s", result);
            }
            free(image);
        }
    }
}
