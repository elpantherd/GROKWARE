from google.cloud import speech
import pyaudio
import config
import logging
import threading
import queue

logger = logging.getLogger(__name__)

class SpeechToTextEngine:
    def __init__(self, language_code=config.GOOGLE_STT_LANGUAGE_CODE):
        self.client = speech.SpeechClient()
        self.language_code = language_code
        self.streaming_config = speech.RecognitionConfig(
            encoding=speech.RecognitionConfig.AudioEncoding.LINEAR16,
            sample_rate_hertz=config.AUDIO_RATE,
            language_code=self.language_code,
            enable_automatic_punctuation=True,
            # model="telephony" or "command_and_search" or "latest_long" might be better
            # use_enhanced=True # If you have enhanced models enabled
        )
        self.is_listening = False
        self.audio_interface = pyaudio.PyAudio()
        self.audio_stream = None
        self._audio_queue = queue.Queue()
        self.transcript_callback = None

    def _microphone_stream_generator(self):
        """Yields audio chunks from the microphone."""
        while self.is_listening:
            chunk = self._audio_queue.get()
            if chunk is None: # Sentinel to stop
                return
            data = [chunk]
            # logger.debug("Yielding audio chunk for STT")
            yield speech.StreamingRecognizeRequest(audio_content=b"".join(data))


    def _audio_callback(self, in_data, frame_count, time_info, status):
        """Continuously collect data from the audio stream into the queue."""
        self._audio_queue.put(in_data)
        return (None, pyaudio.paContinue)

    def start_listening(self, callback_on_transcript):
        if self.is_listening:
            logger.warning("Already listening.")
            return

        self.transcript_callback = callback_on_transcript
        self.is_listening = True
        self._audio_queue = queue.Queue() # Clear previous queue

        try:
            self.audio_stream = self.audio_interface.open(
                format=config.AUDIO_FORMAT,
                channels=config.AUDIO_CHANNELS,
                rate=config.AUDIO_RATE,
                input=True,
                frames_per_buffer=config.AUDIO_CHUNK_SIZE,
                stream_callback=self._audio_callback
            )
            self.audio_stream.start_stream()
            logger.info("Microphone stream started.")

            requests = self._microphone_stream_generator()
            streaming_recognize_config = speech.StreamingRecognitionConfig(
                config=self.streaming_config,
                interim_results=True # Get interim results for faster feedback
            )
            
            # This will run in a new thread to avoid blocking
            self.stt_thread = threading.Thread(target=self._process_recognition, args=(requests, streaming_recognize_config))
            self.stt_thread.daemon = True
            self.stt_thread.start()
            logger.info("STT processing thread started.")

        except Exception as e:
            logger.error(f"Error starting microphone stream: {e}")
            self.is_listening = False
            if self.audio_stream:
                self.audio_stream.stop_stream()
                self.audio_stream.close()
            return

    def _process_recognition(self, requests, streaming_recognize_config):
        try:
            responses = self.client.streaming_recognize(
                config=streaming_recognize_config,
                requests=requests,
            )
            self._listen_print_loop(responses)
        except Exception as e:
            logger.error(f"STT recognition error: {e}")
            # This can happen if the stream times out (e.g. no speech for a while)
            # Or API errors.
        finally:
            logger.info("STT processing thread finished.")
            # Ensure listening stops cleanly if thread exits unexpectedly
            if self.is_listening: # If it wasn't stopped explicitly
                 self.stop_listening(silent=True)


    def _listen_print_loop(self, responses):
        num_chars_printed = 0
        for response in responses:
            if not self.is_listening: # Check if stop_listening was called
                break

            if not response.results:
                continue

            result = response.results[0]
            if not result.alternatives:
                continue

            transcript = result.alternatives[0].transcript

            if result.is_final:
                logger.info(f"Final transcript: {transcript}")
                if self.transcript_callback:
                    self.transcript_callback(transcript, is_final=True)
                num_chars_printed = 0 # Reset for next utterance
            else:
                # logger.debug(f"Interim transcript: {transcript}")
                if self.transcript_callback:
                    self.transcript_callback(transcript, is_final=False)
                # Overwrite interim result (optional, for console debug)
                # overwrite_chars = " " * (num_chars_printed - len(transcript))
                # print(f"{transcript}{overwrite_chars}\r", end="")
                num_chars_printed = len(transcript)
        logger.info("STT response loop ended.")


    def stop_listening(self, silent=False):
        if not self.is_listening:
            if not silent: logger.warning("Not currently listening.")
            return

        self.is_listening = False
        if self.audio_stream:
            self.audio_stream.stop_stream()
            self.audio_stream.close()
            self.audio_stream = None
            logger.info("Microphone stream stopped and closed.")
        
        self._audio_queue.put(None) # Sentinel to stop generator

        if hasattr(self, 'stt_thread') and self.stt_thread.is_alive():
            logger.info("Waiting for STT thread to join...")
            self.stt_thread.join(timeout=2.0) # Wait for thread to finish
            if self.stt_thread.is_alive():
                logger.warning("STT thread did not join in time.")
        
        logger.info("Stopped listening.")
        self.transcript_callback = None


    def close(self):
        self.stop_listening(silent=True)
        self.audio_interface.terminate()
        logger.info("SpeechToTextEngine closed.")


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG) # DEBUG for more verbose output
    
    stt = SpeechToTextEngine()

    def handle_transcript(text, is_final):
        if is_final:
            print(f"\nFINAL: {text}")
            # In a real app, you might stop listening after a final transcript
            # or wait for a pause. For this test, we'll let it run.
        else:
            print(f"INTERIM: {text}\r", end="")

    try:
        stt.start_listening(handle_transcript)
        print("Listening... Press Ctrl+C to stop.")
        while True:
            time.sleep(0.1) # Keep main thread alive
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        stt.close()