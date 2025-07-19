#ifndef SIGN_LANGUAGE_MODEL_H
#define SIGN_LANGUAGE_MODEL_H

#include <stdint.h>

bool tflite_init();
// Returns index of detected class, or -1 on error/no detection
// `scores` array will be filled with probabilities if provided (size should be TFLITE_NUM_CLASSES)
int tflite_predict(uint8_t* image_data, float* scores = nullptr); 
const char* get_class_label(int class_index); // Maps index to string like "Hello", "Thank You"

#endif // SIGN_LANGUAGE_MODEL_H