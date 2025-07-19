from google.cloud import texttospeech
import pygame # For audio playback, simple alternative to mpg123
import config
import logging
import io

logger = logging.getLogger(__name__)

class TextToSpeechEngine:
    def __init__(self, language_code=config.GOOGLE_TTS_LANGUAGE_CODE, voice_name=None):
        self.client = texttospeech.TextToSpeechClient()
        self.voice_config = texttospeech.VoiceSelectionParams(
            language_code=language_code
            # name='en-US-Wavenet-D' # Example Wavenet voice for higher quality
            # ssml_gender=texttospeech.SsmlVoiceGender.NEUTRAL
        )
        if voice_name: # e.g. 'en-US-Standard-C'
            self.voice_config.name = voice_name

        self.audio_config = texttospeech.AudioConfig(
            audio_encoding=texttospeech.AudioEncoding.MP3 # Or LINEAR16 for wav
        )
        pygame.mixer.init(frequency=config.AUDIO_RATE) # Initialize mixer with sample rate
        logger.info("TextToSpeechEngine Initialized. Pygame mixer ready.")

    def speak(self, text):
        if not text:
            logger.warning("TTS: No text provided to speak.")
            return

        synthesis_input = texttospeech.SynthesisInput(text=text)
        try:
            response = self.client.synthesize_speech(
                input=synthesis_input, voice=self.voice_config, audio_config=self.audio_config
            )
            
            # Play the audio using pygame
            audio_fp = io.BytesIO(response.audio_content)
            pygame.mixer.music.load(audio_fp)
            pygame.mixer.music.play()
            logger.info(f"Speaking: {text}")
            # Wait for playback to finish (optional, can be blocking)
            # while pygame.mixer.music.get_busy():
            #     pygame.time.Clock().tick(10)
        except Exception as e:
            logger.error(f"Error during TTS synthesis or playback: {e}")

    def is_speaking(self):
        return pygame.mixer.music.get_busy()

    def stop_speaking(self):
        if pygame.mixer.music.get_busy():
            pygame.mixer.music.stop()
            logger.info("TTS playback stopped.")
            
    def close(self):
        pygame.mixer.quit()
        logger.info("TextToSpeechEngine closed, pygame mixer quit.")


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    tts = TextToSpeechEngine()
    
    print("Testing TTS...")
    tts.speak("Hello, this is a test of the Grokcom text to speech system.")
    while tts.is_speaking():
        pygame.time.Clock().tick(10)
    
    tts.speak("Speech synthesis is working correctly.")
    while tts.is_speaking():
        pygame.time.Clock().tick(10)

    tts.close()
    print("TTS Test complete.")