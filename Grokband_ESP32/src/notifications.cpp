#include "notifications.h"
#include "config.h"
#include <Arduino.h> // For pinMode, digitalWrite, delay

void notification_init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Off
    pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Off
    Serial.println("Notification Handler Initialized");
}

void notification_alert_message(bool long_buzz) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(long_buzz ? 500 : 150);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
}

void notification_led_on() {
    digitalWrite(LED_PIN, HIGH);
}

void notification_led_off() {
    digitalWrite(LED_PIN, LOW);
}

void notification_vibrate(int duration_ms) {
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
}