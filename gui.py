Copyright (c) 2025 Team TECHSMITHS. All rights reserved.
See the LICENSE file for terms of use.
# gui.py
import sys
from PyQt5.QtWidgets import (QMainWindow, QTextEdit, QPushButton, QVBoxLayout, QHBoxLayout, 
                             QWidget, QLabel, QProgressBar, QComboBox)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont
import logging

logger = logging.getLogger("grokcom")

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Grokcom Interface")
        self.setGeometry(100, 100, 800, 600)
        logger.info("Initializing enhanced GUI")

        # Main widget and layout
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QVBoxLayout(self.central_widget)

        # Text area
        self.text_area = QTextEdit()
        self.text_area.setReadOnly(True)
        self.text_area.setFont(QFont("Arial", 14))
        self.text_area.setStyleSheet("""
            background-color: #f0f0f0;
            color: #333333;
            border: 1px solid #cccccc;
            padding: 5px;
        """)
        self.main_layout.addWidget(self.text_area)

        # Status bar
        self.status_label = QLabel("System Status: Idle")
        self.status_label.setStyleSheet("font-size: 12px; color: #666666;")
        self.main_layout.addWidget(self.status_label)

        self.memory_bar = QProgressBar()
        self.memory_bar.setMaximum(100)
        self.memory_bar.setStyleSheet("""
            QProgressBar { border: 1px solid grey; background-color: #ffffff; }
            QProgressBar::chunk { background-color: #4CAF50; }
        """)
        self.main_layout.addWidget(self.memory_bar)

        # Control buttons layout
        self.button_layout = QHBoxLayout()

        self.clear_button = QPushButton("Clear Text")
        self.clear_button.setStyleSheet("""
            QPushButton { background-color: #4CAF50; color: white; font-size: 16px; padding: 5px; }
            QPushButton:hover { background-color: #45a049; }
        """)
        self.clear_button.clicked.connect(self.clear_text)
        self.button_layout.addWidget(self.clear_button)

        self.mute_button = QPushButton("Mute Audio")
        self.mute_button.setStyleSheet("""
            QPushButton { background-color: #f44336; color: white; font-size: 16px; padding: 5px; }
            QPushButton:hover { background-color: #da190b; }
        """)
        self.mute_button.clicked.connect(self.toggle_mute)
        self.button_layout.addWidget(self.mute_button)

        self.theme_combo = QComboBox()
        self.theme_combo.addItems(["Light", "Dark"])
        self.theme_combo.setStyleSheet("font-size: 14px;")
        self.theme_combo.currentTextChanged.connect(self.change_theme)
        self.button_layout.addWidget(self.theme_combo)

        self.main_layout.addLayout(self.button_layout)

        # Variables
        self.is_muted = False
        self.theme = "Light"

        # Timer for memory usage update
        self.memory_timer = QTimer()
        self.memory_timer.timeout.connect(self.update_memory_usage)
        self.memory_timer.start(5000)  # Update every 5 seconds

    def clear_text(self):
        self.text_area.clear()
        logger.info("Text area cleared")
        self.status_label.setText("System Status: Text Cleared")

    def toggle_mute(self):
        self.is_muted = not self.is_muted
        self.mute_button.setText("Unmute Audio" if self.is_muted else "Mute Audio")
        logger.info(f"Audio {'muted' if self.is_muted else 'unmuted'}")
        self.status_label.setText(f"System Status: Audio {'Muted' if self.is_muted else 'Unmuted'}")

    def change_theme(self, theme):
        self.theme = theme
        if theme == "Dark":
            self.text_area.setStyleSheet("""
                background-color: #333333;
                color: #ffffff;
                border: 1px solid #555555;
                padding: 5px;
            """)
            self.central_widget.setStyleSheet("background-color: #222222;")
            self.status_label.setStyleSheet("font-size: 12px; color: #aaaaaa;")
        else:
            self.text_area.setStyleSheet("""
                background-color: #f0f0f0;
                color: #333333;
                border: 1px solid #cccccc;
                padding: 5px;
            """)
            self.central_widget.setStyleSheet("background-color: #ffffff;")
            self.status_label.setStyleSheet("font-size: 12px; color: #666666;")
        logger.info(f"Theme changed to {theme}")

    def update_memory_usage(self):
        import psutil
        memory_percent = psutil.virtual_memory().percent
        self.memory_bar.setValue(int(memory_percent))
        self.status_label.setText(f"System Status: Memory Usage {memory_percent:.1f}%")

    def display_text(self, text):
        if not self.is_muted:
            self.text_area.append(f"Received: {text}")
            self.text_area.ensureCursorVisible()
            logger.info(f"Displaying text: {text}")
            self.status_label.setText("System Status: Text Received")

if __name__ == "__main__":
    from PyQt5.QtWidgets import QApplication
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
