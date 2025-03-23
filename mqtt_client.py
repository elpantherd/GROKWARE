Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
# mqtt_client.py
import paho.mqtt.client as mqtt
from PyQt5.QtCore import QObject, pyqtSignal
import time
import random
import logging

logger = logging.getLogger("grokcom")

class MQTTClient(QObject):
    message_received = pyqtSignal(str)

    def __init__(self, broker, port, topics):
        super().__init__()
        self.broker = broker
        self.port = port
        self.topics = topics
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        self.connected = False
        self.retry_interval = 5
        self.max_interval = 60
        self.retry_count = 0
        self.max_retries = 10

    def start(self):
        logger.info("Starting MQTT client")
        self.connect_to_broker()

    def connect_to_broker(self):
        while not self.connected and self.retry_count < self.max_retries:
            try:
                logger.info(f"Attempting to connect to {self.broker}:{self.port} (Attempt {self.retry_count + 1}/{self.max_retries})")
                self.client.connect(self.broker, self.port, keepalive=60)
                self.client.loop_start()
                self.connected = True
                self.retry_count = 0
                logger.info("Successfully connected to MQTT broker")
                break
            except Exception as e:
                logger.error(f"Connection failed: {e}")
                self.retry_count += 1
                sleep_time = min(self.retry_interval * (2 ** self.retry_count), self.max_interval)
                sleep_time += random.uniform(0, 5)  # Add jitter
                logger.info(f"Retrying in {sleep_time:.2f} seconds")
                time.sleep(sleep_time)

        if not self.connected:
            logger.error("Max retries reached, giving up on MQTT connection")

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            logger.info("MQTT connected successfully")
            self.connected = True
            for topic in self.topics:
                self.client.subscribe(topic)
                logger.info(f"Subscribed to topic: {topic}")
        else:
            logger.error(f"Failed to connect with return code {rc}")
            self.connected = False

    def on_message(self, client, userdata, msg):
        payload = msg.payload.decode("utf-8")
        logger.info(f"Message received on {msg.topic}: {payload}")
        self.message_received.emit(payload)

    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        logger.warning(f"Disconnected from MQTT broker with code {rc}")
        if rc != 0:  # Unexpected disconnect
            self.retry_count = 0
            logger.info("Attempting to reconnect...")
            self.connect_to_broker()

    def publish(self, topic, message):
        if self.connected:
            result = self.client.publish(topic, message)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.info(f"Published to {topic}: {message}")
            else:
                logger.error(f"Failed to publish to {topic}: {result.rc}")
        else:
            logger.warning("Cannot publish, not connected to broker")

    def stop(self):
        self.client.loop_stop()
        self.client.disconnect()
        logger.info("MQTT client stopped")
