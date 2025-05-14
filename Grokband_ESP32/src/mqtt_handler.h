#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <WiFi.h>

void setup_wifi();
void setup_mqtt(MQTT_CALLBACK_SIGNATURE); // Pass callback for received messages
void mqtt_reconnect();
void mqtt_loop();
void mqtt_publish(const char* topic, const char* payload);
bool is_mqtt_connected();

#endif // MQTT_HANDLER_H