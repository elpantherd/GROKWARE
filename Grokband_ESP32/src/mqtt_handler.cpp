#include "mqtt_handler.h"
#include "config.h"
#include <WiFiClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup_mqtt(MQTT_CALLBACK_SIGNATURE callback) {
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    mqttClient.setCallback(callback);
    mqtt_reconnect();
}

void mqtt_reconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("connected");
            mqttClient.subscribe(MQTT_TOPIC_SPEECH_TO_SIGN);
            Serial.print("Subscribed to: "); Serial.println(MQTT_TOPIC_SPEECH_TO_SIGN);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqtt_reconnect();
    }
    mqttClient.loop();
}

void mqtt_publish(const char* topic, const char* payload) {
    if (mqttClient.connected()) {
        mqttClient.publish(topic, payload);
        Serial.print("MQTT Published ["); Serial.print(topic); Serial.print("]: "); Serial.println(payload);
    } else {
        Serial.println("MQTT not connected. Cannot publish.");
    }
}

bool is_mqtt_connected() {
    return mqttClient.connected();
}