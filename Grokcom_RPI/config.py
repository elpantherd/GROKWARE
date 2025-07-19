# MQTT Configuration
MQTT_BROKER_IP = "localhost"  # Or the IP of your RPi if broker is on another machine
MQTT_BROKER_PORT = 1883
MQTT_CLIENT_ID_GROKCOM = "grokcom_rpi"

# MQTT Topics (Consistent with Grokband)
MQTT_TOPIC_SIGN_TO_TEXT = "grokware/grokband/sign_to_text" # Grokcom subscribes
MQTT_TOPIC_SPEECH_TO_TEXT = "grokware/grokcom/speech_to_text" # Grokcom publishes
MQTT_TOPIC_QUICK_RESPONSE = "grokware/grokband/quick_response" # Grokcom subscribes

# Google Cloud Credentials
# IMPORTANT: Set the GOOGLE_APPLICATION_CREDENTIALS environment variable
# to the path of your JSON service account key file.
# export GOOGLE_APPLICATION_CREDENTIALS="/path/to/your/keyfile.json"
GOOGLE_TTS_LANGUAGE_CODE = "en-US"
GOOGLE_STT_LANGUAGE_CODE = "en-US"

# Audio settings
AUDIO_CHUNK_SIZE = 1024
AUDIO_FORMAT = pyaudio.paInt16 # Corresponds to 16-bit samples
AUDIO_CHANNELS = 1
AUDIO_RATE = 16000 # Sample rate, 16kHz is common for speech recognition

# UI Settings
DISPLAY_WIDTH = 800 # Adjust to your 5.7" TFT LCD resolution
DISPLAY_HEIGHT = 480 # Adjust