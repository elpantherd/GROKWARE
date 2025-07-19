#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// MQTT Broker
#define MQTT_BROKER_IP "YOUR_MQTT_BROKER_IP" // e.g., IP of your Raspberry Pi
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "grokband_esp32"

// MQTT Topics (Consistent with Grokcom)
#define MQTT_TOPIC_SIGN_TO_TEXT "grokware/grokband/sign_to_text"
#define MQTT_TOPIC_SPEECH_TO_SIGN "grokware/grokcom/speech_to_text" // Grokband subscribes to this
#define MQTT_TOPIC_QUICK_RESPONSE "grokware/grokband/quick_response"

// Hardware Pins (ADJUST THESE TO YOUR ACTUAL WIRING)

// OV2640 Camera Pins (Example for AI Thinker ESP32-CAM, adapt for Pico D4 connection)
// These need to be ESP-IDF style pin numbers
#define CAM_PIN_PWDN   -1 // 연결되지 않은 경우 -1
#define CAM_PIN_RESET  -1 // ESP32-CAM 보드에서는 연결되지 않음
#define CAM_PIN_XCLK   GPIO_NUM_0
#define CAM_PIN_SIOD   GPIO_NUM_26
#define CAM_PIN_SIOC   GPIO_NUM_27
#define CAM_PIN_D7     GPIO_NUM_35
#define CAM_PIN_D6     GPIO_NUM_34
#define CAM_PIN_D5     GPIO_NUM_39
#define CAM_PIN_D4     GPIO_NUM_36
#define CAM_PIN_D3     GPIO_NUM_21
#define CAM_PIN_D2     GPIO_NUM_19
#define CAM_PIN_D1     GPIO_NUM_18
#define CAM_PIN_D0     GPIO_NUM_5
#define CAM_PIN_VSYNC  GPIO_NUM_25
#define CAM_PIN_HREF   GPIO_NUM_23
#define CAM_PIN_PCLK   GPIO_NUM_22

// OLED Display (Assuming I2C, adapt for SPI if necessary)
#define OLED_SDA_PIN GPIO_NUM_4 // Example
#define OLED_SCL_PIN GPIO_NUM_15 // Example
#define OLED_RST_PIN GPIO_NUM_16 // Example, or -1 if not used

// PC311 Rotary Encoder Pins
#define ROTARY_ENCODER_A_PIN  GPIO_NUM_32 // Example DT
#define ROTARY_ENCODER_B_PIN  GPIO_NUM_33 // Example CLK
#define ROTARY_ENCODER_SW_PIN GPIO_NUM_25 // Example SW (Button)

// LED Pin
#define LED_PIN GPIO_NUM_2 // Example, onboard LED if available or external

// Vibration Motor Pin
#define VIBRATION_MOTOR_PIN GPIO_NUM_12 // Example

// TFLite Model settings
#define TFLITE_MODEL_INPUT_WIDTH 96   // Example, depends on your model
#define TFLITE_MODEL_INPUT_HEIGHT 96  // Example
#define TFLITE_MODEL_INPUT_CHANNELS 1 // Example (grayscale) or 3 (RGB)
#define TFLITE_NUM_CLASSES 10         // Example, number of sign language gestures

// Quick Responses
const char* quick_responses[] = {"Yes", "No", "Okay", "Thank You", "Hello"};
#define NUM_QUICK_RESPONSES (sizeof(quick_responses) / sizeof(char*))

#endif // CONFIG_H