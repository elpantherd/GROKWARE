# GROKWARE
Grokware: Real-time sign language to speech and voice to text translation for inclusive communication.

A sleek, innovative system featuring Grokband and Grokcom, Grokware effortlessly translates sign language into text and audio, while converting spoken words back to text—bridging the gap for seamless, universal connection.
# Grokband and Grokcom

This project consists of two proprietary devices: **Grokband** and **Grokcom**, designed for bidirectional communication through sign language and voice.

## Grokband

Grokband is a wristband based on the ESP32 Pico D4 microcontroller. It features:
- An OV2640 camera module for capturing real-time two-hand sign language.
- A circular OLED display showing quick responses navigated via a PC311 rotary encoder.
- An LED and vibration motor for feedback on received messages.
- Converts sign language to text and communicates with Grokcom via MQTT.

### Hardware Requirements
- ESP32 Pico D4
- OV2640 camera module
- Circular OLED display
- PC311 rotary encoder
- LED
- Vibration motor
- Power management IC

### Software Requirements
- FreeRTOS
- ESP-IDF
- C/C++
- Arducam library
- Rotary encoder library
- LVGL library
- MQTT
- TensorFlow Lite

### Setup
1. Install ESP-IDF (follow official documentation).
2. Compile and flash the firmware using `idf.py build flash`.
3. Configure Wi-Fi credentials in `main.c` (add Wi-Fi initialization code).
4. Ensure all hardware is connected as per pin definitions in `config.h`.

## Grokcom

Grokcom is a wall-mounted standalone device based on Raspberry Pi with a 5.7" TFT LCD display. It:
- Displays text received from Grokband.
- Converts received text to audio using Google Text-to-Speech.
- Captures real-time audio, converts it to text, and sends it to Grokband via MQTT.

### Hardware Requirements
- Raspberry Pi (e.g., 4GB)
- 5.7" TFT LCD display

### Software Requirements
- Raspberry Pi OS
- Python
- C++ (for TLS encryption)
- PyQt
- Standard Touchscreen LCD support
- Google Text-to-Speech API
- TLS encryption libraries
- MQTT (paho-mqtt)

### Setup
1. Install Raspberry Pi OS on the Raspberry Pi.
2. Install dependencies: `pip install -r requirements.txt`.
3. Install `mpg123`: `sudo apt-get install mpg123`.
4. Run the application: `python main.py`.

## Usage
- **Grokband**: 
  - Rotate the encoder to navigate quick responses on the OLED; press to send.
  - Perform sign language gestures in front of the camera to send text to Grokcom.
  - LED and vibration motor activate on receiving messages from Grokcom.
- **Grokcom**: 
  - Speak to send text to Grokband.
  - Received sign language text is displayed on the LCD and spoken aloud.

## License
This project is proprietary and not open source. Please see the [LICENSE](LICENSE) file for terms of use.

## Contact
For support, contact TEAM TECHSMITHS at dthayalan760@gmail.com.
