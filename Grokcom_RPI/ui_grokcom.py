from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QTextEdit, QPushButton, QHBoxLayout
from PyQt5.QtCore import pyqtSignal, Qt, QTimer
from PyQt5.QtGui import QFont
import config

class GrokcomUI(QWidget):
    # Signals for interaction with main logic
    start_listening_signal = pyqtSignal()
    stop_listening_signal = pyqtSignal()
    send_message_signal = pyqtSignal(str) # For manually typed messages if needed

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Grokcom Interface")
        self.setGeometry(50, 50, config.DISPLAY_WIDTH, config.DISPLAY_HEIGHT) # Adjust as needed
        # self.showFullScreen() # For kiosk mode on RPi display

        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        # Status Label
        self.status_label = QLabel("Status: Initializing...")
        self.status_label.setFont(QFont("Arial", 18))
        self.status_label.setAlignment(Qt.AlignCenter)
        self.layout.addWidget(self.status_label)

        # Conversation Display Area
        self.conversation_display = QTextEdit()
        self.conversation_display.setReadOnly(True)
        self.conversation_display.setFont(QFont("Arial", 16))
        self.layout.addWidget(self.conversation_display, stretch=3) # Give more space

        # Interim Transcript Display (for STT)
        self.interim_transcript_label = QLabel("Listening...")
        self.interim_transcript_label.setFont(QFont("Arial", 14, QFont.BoldItalic))
        self.interim_transcript_label.setAlignment(Qt.AlignCenter)
        self.interim_transcript_label.setFixedHeight(40)
        self.layout.addWidget(self.interim_transcript_label)
        self.interim_transcript_label.hide() # Initially hidden

        # Control Buttons
        self.controls_layout = QHBoxLayout()
        self.listen_button = QPushButton("Start Listening (Push to Talk)")
        self.listen_button.setFont(QFont("Arial", 16))
        self.listen_button.setFixedHeight(60)
        # self.listen_button.setCheckable(True) # For toggle behavior
        # self.listen_button.toggled.connect(self.on_listen_button_toggled)
        self.listen_button.pressed.connect(self.on_listen_button_pressed)
        self.listen_button.released.connect(self.on_listen_button_released)

        self.controls_layout.addWidget(self.listen_button)
        self.layout.addLayout(self.controls_layout)
        
        self.is_ptt_active = False # Push-to-talk state

    def on_listen_button_pressed(self):
        if not self.is_ptt_active:
            self.is_ptt_active = True
            self.listen_button.setText("Listening...")
            self.listen_button.setStyleSheet("background-color: lightgreen;")
            self.interim_transcript_label.setText("...")
            self.interim_transcript_label.show()
            self.start_listening_signal.emit()

    def on_listen_button_released(self):
        if self.is_ptt_active:
            self.is_ptt_active = False
            self.listen_button.setText("Start Listening (Push to Talk)")
            self.listen_button.setStyleSheet("") # Reset style
            # self.interim_transcript_label.hide() # Keep it visible briefly or clear it
            self.stop_listening_signal.emit()


    # --- Methods to be called from main_app.py ---
    def update_status(self, text):
        self.status_label.setText(f"Status: {text}")

    def add_conversation_message(self, sender, message):
        # Simple bolding for sender
        self.conversation_display.append(f"<b>{sender}:</b> {message}")
        # Scroll to bottom
        self.conversation_display.verticalScrollBar().setValue(
            self.conversation_display.verticalScrollBar().maximum()
        )

    def display_interim_transcript(self, text):
        if self.is_ptt_active: # Only show if PTT is active
            self.interim_transcript_label.setText(text)
            self.interim_transcript_label.show()

    def clear_interim_transcript(self):
        self.interim_transcript_label.setText("...") # Or hide it
        # self.interim_transcript_label.hide()


if __name__ == '__main__':
    import sys
    app = QApplication(sys.argv)
    main_window = GrokcomUI()
    main_window.show()

    # Test methods
    main_window.update_status("Connected")
    main_window.add_conversation_message("Grokband", "Hello from the wristband!")
    main_window.add_conversation_message("Grokcom", "Hi Grokband, I heard you!")
    
    def test_interim():
        main_window.display_interim_transcript("User is speaking something interesting...")
    
    if hasattr(main_window.listen_button, 'pressed'): # If PTT
        main_window.listen_button.pressed.connect(lambda: main_window.interim_transcript_label.show())
        main_window.listen_button.released.connect(lambda: QTimer.singleShot(1000, main_window.clear_interim_transcript)) # Clear after 1s

    # QTimer.singleShot(2000, test_interim)
    # QTimer.singleShot(4000, lambda: main_window.display_interim_transcript("And now for something completely different."))
    # QTimer.singleShot(6000, main_window.clear_interim_transcript)


    sys.exit(app.exec_())