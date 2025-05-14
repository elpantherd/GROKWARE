import paho.mqtt.client as mqtt
import config
import logging

logger = logging.getLogger(__name__)

class MQTTClient:
    def __init__(self, on_message_callback=None):
        self.client = mqtt.Client(client_id=config.MQTT_CLIENT_ID_GROKCOM)
        self.on_message_callback = on_message_callback

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message_internal
        self.client.on_disconnect = self.on_disconnect

        self.connected = False

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            logger.info(f"Connected to MQTT Broker: {config.MQTT_BROKER_IP}")
            self.connected = True
            # Subscribe to topics from Grokband
            client.subscribe(config.MQTT_TOPIC_SIGN_TO_TEXT)
            logger.info(f"Subscribed to {config.MQTT_TOPIC_SIGN_TO_TEXT}")
            client.subscribe(config.MQTT_TOPIC_QUICK_RESPONSE)
            logger.info(f"Subscribed to {config.MQTT_TOPIC_QUICK_RESPONSE}")
        else:
            logger.error(f"Failed to connect to MQTT, return code {rc}")
            self.connected = False

    def on_message_internal(self, client, userdata, msg):
        payload_str = msg.payload.decode('utf-8')
        logger.info(f"MQTT Received on [{msg.topic}]: {payload_str}")
        if self.on_message_callback:
            self.on_message_callback(msg.topic, payload_str)

    def on_disconnect(self, client, userdata, rc):
        logger.warning(f"Disconnected from MQTT Broker with rc: {rc}")
        self.connected = False
        # Implement reconnection logic if desired

    def connect(self):
        try:
            self.client.connect(config.MQTT_BROKER_IP, config.MQTT_BROKER_PORT, 60)
            self.client.loop_start()  # Start a background thread for network traffic
        except Exception as e:
            logger.error(f"MQTT connection error: {e}")
            self.connected = False


    def publish(self, topic, payload):
        if self.connected:
            self.client.publish(topic, payload)
            logger.info(f"MQTT Published [{topic}]: {payload}")
        else:
            logger.warning("MQTT not connected. Cannot publish.")

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        logger.info("MQTT client disconnected.")

if __name__ == '__main__':
    # Example Usage
    logging.basicConfig(level=logging.INFO)
    
    def my_callback(topic, payload):
        print(f"Callback received: Topic={topic}, Payload={payload}")

    mqtt_service = MQTTClient(on_message_callback=my_callback)
    mqtt_service.connect()

    import time
    try:
        while True:
            # mqtt_service.publish("test/grokcom", "Hello from Grokcom standalone test")
            time.sleep(5)
    except KeyboardInterrupt:
        print("Exiting")
    finally:
        mqtt_service.disconnect()