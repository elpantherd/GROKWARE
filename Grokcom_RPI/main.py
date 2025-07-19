import sys
import logging
from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QObject, pyqtSignal, QThread, QTimer

import config
from ui_grokcom import GrokcomUI
from mqtt_client import MQTTClient
from speech_to_text import SpeechToTextEngine
from text_to_speech import TextToSpeechEngine

# Configure logging
logging.basicConfig(
    level=logging.INFO, # Change to DEBUG for more verbosity
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout) # Output to console
        # logging.FileHandler("grokcom.log") # Optionally log to a file
    ]
)
logger = logging.getLogger(__name__)

class GrokcomApp(QObject):
    # Signals to update UI from non-UI threads
    ui_update_status_signal = pyqtSignal(str)
    ui_add_conversation_signal = pyqtSignal(str, str) # sender, message
    ui_display_interim_transcript_signal = pyqtSignal(str)
    ui_clear_interim_transcript_signal = pyqtSignal()

    def __init__(self):
        super().__init__()
        self.ui = GrokcomUI()

        # Initialize core components
        self.mqtt = MQTTClient(on_message_callback=self.handle_mqtt_message)
        self.stt = SpeechToTextEngine()
        self.tts = TextToSpeechEngine()

        # Connect UI signals to backend methods
        self.ui.start_listening_signal.connect(self.start_stt_listening)
        self.ui.stop_listening_signal.connect(self.stop_stt_listening_and_process)
        # self.ui.send_message_signal.connect(self.send_typed_message) # If you add a text input

        # Connect backend signals to UI slots
        self.ui_update_status_signal.connect(self.ui.update_status)
        self.ui_add_conversation_signal.connect(self.ui.add_conversation_message)
        self.ui_display_interim_transcript_signal.connect(self.ui.display_interim_transcript)
        self.ui_clear_interim_transcript_signal.connect(self.ui.clear_interim_transcript)
        
        self.last_final_transcript = ""

    def start(self):
        logger.info("Grokcom Application Starting...")
        self.ui_update_status_signal.emit("Initializing...")
        self.ui.show()

        self.mqtt.connect()
        if self.mqtt.connected:
            self.ui_update_status_signal.emit("MQTT Connected")
        else:
            self.ui_update_status_signal.emit("MQTT Connection Failed")
        
        # Initial message
        self.ui_add_conversation_signal.emit("System", "Welcome to Grokcom!")

    def handle_mqtt_message(self, topic, payload):
        logger.info(f"MainApp: MQTT message received on topic '{topic}'")
        processed = False
        if topic == config.MQTT_TOPIC_SIGN_TO_TEXT:
            self.ui_add_conversation_signal.emit("Grokband (Sign)", payload)
            self.tts.speak(f"Received sign: {payload}")
            processed = True
        elif topic == config.MQTT_TOPIC_QUICK_RESPONSE:
            self.ui_add_conversation_signal.emit("Grokband (Quick)", payload)
            self.tts.speak(f"Quick response: {payload}")
            processed = True
        
        if not processed:
            logger.warning(f"Unhandled MQTT topic: {topic}")

    def start_stt_listening(self):
        logger.info("UI requested STT start")
        if self.tts.is_speaking():
            self.tts.stop_speaking() # Stop TTS if it's talking
            QTimer.singleShot(300, lambda: self.stt.start_listening(self.handle_stt_transcript)) # Small delay
        else:
            self.stt.start_listening(self.handle_stt_transcript)
        self.ui_update_status_signal.emit("Listening...")

    def stop_stt_listening_and_process(self):
        logger.info("UI requested STT stop")
        self.stt.stop_listening()
        # The final transcript is handled by handle_stt_transcript's is_final=True
        # We might want to send the self.last_final_transcript here if it hasn't been sent
        # This depends on how quickly STT finalizes.
        # For PTT, the final result from handle_stt_transcript should be enough.
        self.ui_update_status_signal.emit( "Processing..." if self.last_final_transcript else "Ready")
        # QTimer.singleShot(1000, self.ui_clear_interim_transcript_signal.emit) # Clear after a bit
        self.ui_clear_interim_transcript_signal.emit()


    def handle_stt_transcript(self, transcript, is_final):
        if is_final:
            logger.info(f"STT Final Transcript: {transcript}")
            self.last_final_transcript = transcript # Store for potential use
            if transcript.strip(): # Only process non-empty transcripts
                self.ui_add_conversation_signal.emit("You (Voice)", transcript)
                self.mqtt.publish(config.MQTT_TOPIC_SPEECH_TO_TEXT, transcript)
                self.ui_update_status_signal.emit("Ready") # STT processing done
            else:
                 self.ui_update_status_signal.emit("No speech detected or empty.")
            self.ui_clear_interim_transcript_signal.emit()
        else:
            # logger.debug(f"STT Interim Transcript: {transcript}")
            self.ui_display_interim_transcript_signal.emit(transcript)


    def cleanup(self):
        logger.info("Grokcom Application Shutting Down...")
        if self.stt:
            self.stt.close()
        if self.tts:
            self.tts.close()
        if self.mqtt:
            self.mqtt.disconnect()
        logger.info("Cleanup complete.")

def main():
    app = QApplication(sys.argv)
    grokcom_app = GrokcomApp()
    
    try:
        grokcom_app.start()
        exit_code = app.exec_()
    except Exception as e:
        logger.critical(f"Unhandled exception in main: {e}", exc_info=True)
        exit_code = 1 # Indicate error
    finally:
        grokcom_app.cleanup()
        
    sys.exit(exit_code)

if __name__ == '__main__':
    main()