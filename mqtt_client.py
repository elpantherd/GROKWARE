import logging
import paho.mqtt.client as mqtt
from PyQt5.QtCore import pyqtSignal, QObject

logger = logging.getLogger(__name__)

class MQTTClient(QObject):
    message_received = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        # Uncomment for TLS
        # self.client.tls_set(ca_certs="ca.crt", certfile="client.crt", keyfile="client.key")
        logger.info("MQTT client initialized")
    
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            logger.info("Connected to MQTT broker")
            client.subscribe("sign_language_text")
        else:
            logger.error(f"Connection failed with code {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        logger.warning("Disconnected from MQTT broker")
    
    def on_message(self, client, userdata, msg):
        text = msg.payload.decode()
        logger.info(f"Received message: {text}")
        self.message_received.emit(text)
    
    def start(self):
        self.client.connect("broker.example.com", 1883, 60)
        self.client.loop_start()
        logger.info("MQTT client started")
    
    def publish_text(self, text):
        self.client.publish("voice_text", text)
        logger.info(f"Published text: {text}")
