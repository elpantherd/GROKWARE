Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
# config.py
# Configuration settings
MQTT_BROKER = "broker.example.com"
MQTT_PORT = 1883
TOPIC_SIGN_TEXT = "sign_language_text"
TOPIC_VOICE_TEXT = "voice_text"
LOG_FILE = "grokcom.log"
LOG_LEVEL = "INFO"
GUI_UPDATE_INTERVAL = 100  # ms
SPEECH_TIMEOUT = 5  # seconds

# main.py
import sys
import logging
from PyQt5.QtWidgets import QApplication
from gui import MainWindow
from mqtt_client import MQTTClient
from speech import SpeechHandler
import config

# Setup logging
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(config.LOG_FILE),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("grokcom")

class GrokcomApp:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.window = MainWindow()
        self.mqtt_client = MQTTClient(
            config.MQTT_BROKER,
            config.MQTT_PORT,
            [config.TOPIC_SIGN_TEXT, config.TOPIC_VOICE_TEXT]
        )
        self.speech_handler = SpeechHandler()

        # Connect signals
        self.mqtt_client.message_received.connect(self.window.display_text)
        self.mqtt_client.message_received.connect(self.speech_handler.text_to_speech)
        self.speech_handler.text_recognized.connect(self.mqtt_client.publish(config.TOPIC_VOICE_TEXT))

        # Start components
        self.mqtt_client.start()
        self.speech_handler.start()
        self.window.show()

    def run(self):
        logger.info("Starting Grokcom application")
        sys.exit(self.app.exec_())

    def shutdown(self):
        logger.info("Shutting down Grokcom application")
        self.mqtt_client.stop()
        self.speech_handler.stop()

if __name__ == "__main__":
    app = GrokcomApp()
    try:
        app.run()
    except Exception as e:
        logger.error(f"Application crashed: {e}")
    finally:
        app.shutdown()
