#include "sign_language_model.h"
#include "config.h" // For TFLITE_MODEL_* defines

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// For loading model from flash (SPIFFS or LittleFS)
#include "esp_spiffs.h" // Or LittleFS header if you use that
#include <stdio.h> // For FILE* operations

namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input_tensor = nullptr;
    TfLiteTensor* output_tensor = nullptr;

    // Arena for TFLite, size depends on your model.
    // This needs to be tuned! Too small = crash, too large = wasted RAM.
    constexpr int kTensorArenaSize = 90 * 1024; // EXAMPLE: 90KB, VERY model dependent
    uint8_t tensor_arena[kTensorArenaSize];

    // Path to the model file in SPIFFS/LittleFS
    const char* model_path = "/spiffs/sign_model.tflite"; // Ensure this matches your data dir upload
    unsigned char* model_data_buffer = nullptr; // To hold model data read from flash

    // Example class labels - must match your model's output
    const char* class_labels[TFLITE_NUM_CLASSES] = {
        "Sign A", "Sign B", "Sign C", "Help", "Yes", "No", "Hello", "Goodbye", "Thank You", "Eat" // Adjust these!
    };
}

// Helper to load model from flash storage
bool load_model_from_flash() {
    // Mount SPIFFS if not already mounted
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL, // Auto-find SPIFFS partition
      .max_files = 5,
      .format_if_mount_failed = true // Format if corrupted (careful in production)
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) { // ESP_ERR_INVALID_STATE means already mounted
        error_reporter->Report("SPIFFS Mount Failed: %s", esp_err_to_name(ret));
        return false;
    }

    FILE* model_file = fopen(model_path, "rb");
    if (!model_file) {
        error_reporter->Report("Failed to open model file: %s", model_path);
        esp_vfs_spiffs_unregister(NULL); // Unmount on failure to open
        return false;
    }

    fseek(model_file, 0, SEEK_END);
    long model_size = ftell(model_file);
    fseek(model_file, 0, SEEK_SET);

    model_data_buffer = (unsigned char*)malloc(model_size);
    if (!model_data_buffer) {
        error_reporter->Report("Failed to allocate memory for model data");
        fclose(model_file);
        esp_vfs_spiffs_unregister(NULL);
        return false;
    }

    size_t bytes_read = fread(model_data_buffer, 1, model_size, model_file);
    fclose(model_file);
    // Don't unmount SPIFFS yet if other parts of app might use it. Or unmount if done.
    // esp_vfs_spiffs_unregister(NULL); 

    if (bytes_read != model_size) {
        error_reporter->Report("Failed to read entire model file. Read %d, expected %d", bytes_read, model_size);
        free(model_data_buffer);
        model_data_buffer = nullptr;
        return false;
    }
    
    model = tflite::GetModel(model_data_buffer);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report("Model provided is schema version %d not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        free(model_data_buffer); // clean up
        model_data_buffer = nullptr;
        model = nullptr;
        return false;
    }
    error_reporter->Report("Model loaded successfully from flash. Size: %ld bytes", model_size);
    return true;
}


bool tflite_init() {
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    if (!load_model_from_flash()) {
        error_reporter->Report("Failed to load TFLite model from flash.");
        return false; // Critical failure
    }
    // If model is null here, load_model_from_flash failed and reported.
    if (!model) return false;


    // This pulls in all operators. For production, you might want a subset
    // to save space, using tflite::MicroMutableOpResolver.
    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        error_reporter->Report("AllocateTensors() failed");
        return false;
    }

    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);

    // Sanity check tensor dimensions (optional but good)
    if (input_tensor->dims->size < 3 || // B, H, W, C (usually B is 1 for micro)
        input_tensor->dims->data[1] != TFLITE_MODEL_INPUT_HEIGHT ||
        input_tensor->dims->data[2] != TFLITE_MODEL_INPUT_WIDTH ||
        input_tensor->dims->data[3] != TFLITE_MODEL_INPUT_CHANNELS) {
        error_reporter->Report("Bad input tensor parameters in model!");
        return false;
    }
    if (output_tensor->dims->data[1] != TFLITE_NUM_CLASSES) {
         error_reporter->Report("Bad output tensor parameters in model! Expected %d classes, got %d",
                                TFLITE_NUM_CLASSES, output_tensor->dims->data[1]);
        return false;
    }


    error_reporter->Report("TensorFlow Lite Micro Initialized");
    return true;
}

int tflite_predict(uint8_t* image_data, float* scores_out) {
    if (!interpreter || !input_tensor) {
        error_reporter->Report("TFLite not initialized or input tensor is null.");
        return -1;
    }

    // Copy image data to input tensor
    // Model might expect float input between -1 and 1 or 0 and 1.
    // If so, normalization needs to happen here or in preprocess_camera_frame.
    // For uint8 quantized models, direct copy might be fine.
    if (input_tensor->type == kTfLiteUInt8) {
        uint8_t* input_data_ptr = tflite::GetTensorData<uint8_t>(input_tensor);
        memcpy(input_data_ptr, image_data, input_tensor->bytes);
    } else if (input_tensor->type == kTfLiteFloat32) {
        float* input_data_ptr = tflite::GetTensorData<float>(input_tensor);
        // Assuming image_data is uint8, convert and normalize
        for (size_t i = 0; i < input_tensor->bytes / sizeof(float); ++i) {
            // Example normalization: (0 to 255) -> (-1 to 1)
            input_data_ptr[i] = (static_cast<float>(image_data[i]) - 127.5f) / 127.5f;
        }
    } else {
        error_reporter->Report("Unsupported input tensor type: %d", input_tensor->type);
        return -1;
    }


    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        error_reporter->Report("Invoke() failed");
        return -1;
    }

    output_tensor = interpreter->output(0); // Re-get output tensor just in case

    // Process output tensor
    int max_score_index = -1;
    float max_score = -1.0f; // Assuming scores are probabilities (0-1) or logits

    for (int i = 0; i < TFLITE_NUM_CLASSES; ++i) {
        float current_score;
        if (output_tensor->type == kTfLiteUInt8) {
            // Dequantize if necessary. Output scale and zero point are needed.
            // For simplicity, let's assume it gives raw scores or direct class if it's argmax inside model.
            // This part is highly dependent on your model's output layer.
            // If it's logits, you might apply softmax here.
            // If it's probabilities, they might be uint8 representing 0-255.
            uint8_t val = tflite::GetTensorData<uint8_t>(output_tensor)[i];
            current_score = val / 255.0f; // Simple scaling if output is uint8 prob
            // Or use dequantize formula: float real_value = scale * (quantized_value - zero_point);
            // float scale = output_tensor->params.scale;
            // int zero_point = output_tensor->params.zero_point;
            // current_score = scale * (val - zero_point);

        } else if (output_tensor->type == kTfLiteFloat32) {
            current_score = tflite::GetTensorData<float>(output_tensor)[i];
        } else {
            error_reporter->Report("Unsupported output tensor type: %d", output_tensor->type);
            return -1;
        }
        
        if (scores_out) {
            scores_out[i] = current_score;
        }

        if (current_score > max_score) {
            max_score = current_score;
            max_score_index = i;
        }
    }
    
    // Optional: Add a confidence threshold
    // float confidence_threshold = 0.7; // Example
    // if (max_score < confidence_threshold) {
    //     return -1; // Not confident enough
    // }

    return max_score_index;
}

const char* get_class_label(int class_index) {
    if (class_index >= 0 && class_index < TFLITE_NUM_CLASSES) {
        return class_labels[class_index];
    }
    return "Unknown";
}