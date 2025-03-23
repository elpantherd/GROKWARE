Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
import sys
import logging
from PyQt5.QtWidgets import QApplication
from gui import MainWindow
from mqtt_client import MQTTClient
from speech import SpeechHandler

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def main():
    logger.info("Starting Grokcom application")
    app = QApplication(sys.argv)
    
    # Initialize components
    window = MainWindow()
    mqtt_client = MQTTClient()
    speech_handler = SpeechHandler()
    
    # Connect signals
    mqtt_client.message_received.connect(window.display_text)
    mqtt_client.message_received.connect(speech_handler.text_to_speech)
    speech_handler.text_recognized.connect(mqtt_client.publish_text)
    
    # Start components
    mqtt_client.start()
    speech_handler.start()
    window.show()
    
    logger.info("Application initialized")
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
