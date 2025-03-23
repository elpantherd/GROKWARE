Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
# speech.py
from PyQt5.QtCore import QThread, pyqtSignal
import speech_recognition as sr
from gtts import gTTS
import os
import logging
import time

logger = logging.getLogger("grokcom")

class SpeechHandler(QThread):
    text_recognized = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.recognizer = sr.Recognizer()
        self.microphone = sr.Microphone()
        self.running = True
        self.is_muted = False
        self.max_retries = 3
        self.retry_delay = 2  # seconds

    def run(self):
        logger.info("Speech handler thread started")
        while self.running:
            retry_count = 0
            success = False
            with self.microphone as source:
                self.recognizer.adjust_for_ambient_noise(source, duration=1)
                logger.debug("Adjusted for ambient noise")
                while retry_count < self.max_retries and not success and self.running:
                    try:
                        logger.info("Listening for audio input...")
                        audio = self.recognizer.listen(source, timeout=5, phrase_time_limit=10)
                        text = self.recognizer.recognize_google(audio)
                        logger.info(f"Recognized speech: {text}")
                        self.text_recognized.emit(text)
                        success = True
                    except sr.WaitTimeoutError:
                        logger.debug("No speech detected within timeout")
                        break
                    except sr.UnknownValueError:
                        logger.warning("Could not understand audio")
                        retry_count += 1
                        time.sleep(self.retry_delay)
                    except sr.RequestError as e:
                        logger.error(f"Speech recognition request failed: {e}")
                        retry_count += 1
                        time.sleep(self.retry_delay)
                    except Exception as e:
                        logger.error(f"Unexpected error in speech recognition: {e}")
                        retry_count += 1
                        time.sleep(self.retry_delay)

                if not success and retry_count >= self.max_retries:
                    logger.error("Max retries reached for speech recognition")

    def text_to_speech(self, text):
        if self.is_muted:
            logger.debug("Text-to-speech muted, skipping")
            return
        retry_count = 0
        success = False
        while retry_count < self.max_retries and not success:
            try:
                logger.info(f"Converting to speech: {text}")
                tts = gTTS(text=text, lang='en', slow=False)
                audio_file = "output.mp3"
                tts.save(audio_file)
                os.system(f"mpg123 {audio_file} > /dev/null 2>&1")
                os.remove(audio_file)
                success = True
            except Exception as e:
                logger.error(f"Text-to-speech error: {e}")
                retry_count += 1
                time.sleep(self.retry_delay)
        if not success:
            logger.error("Failed to convert text to speech after retries")

    def stop(self):
        self.running = False
        logger.info("Stopping speech handler")
        self.wait()

    def diagnostics(self):
        try:
            with self.microphone as source:
                logger.info("Running microphone diagnostics")
                self.recognizer.adjust_for_ambient_noise(source, duration=1)
                logger.info("Microphone is functioning")
        except Exception as e:
            logger.error(f"Microphone diagnostics failed: {e}")
