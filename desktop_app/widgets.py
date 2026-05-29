from PySide6.QtWidgets import QFrame, QLabel, QVBoxLayout, QHBoxLayout, QProgressBar, QCheckBox, QSlider, QSizePolicy
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFont

class GlassPanel(QFrame):
    """Semi-transparent glassmorphic container with solid neon borders."""
    def __init__(self, parent=None, border_color="#00FFA2"):
        super().__init__(parent)
        self.setStyleSheet(f"""
            GlassPanel {{
                background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0D1520, stop:1 #070B10);
                border: 1px solid {border_color};
                border-radius: 6px;
            }}
        """)

class NeonProgressBar(QProgressBar):
    """Futuristic progress bar styled in premium neon."""
    def __init__(self, parent=None, chunk_color="#00FFA2"):
        super().__init__(parent)
        self.setTextVisible(False)
        self.setFixedHeight(10)
        self.setStyleSheet(f"""
            QProgressBar {{
                background-color: #060A10;
                border: 1px solid #1E2A38;
                border-radius: 5px;
            }}
            QProgressBar::chunk {{
                background-color: {chunk_color};
                border-radius: 4px;
            }}
        """)

class NeonCheckBox(QCheckBox):
    """Custom toggle switch checkbox styled in neon colors."""
    def __init__(self, text="", parent=None, active_color="#00FFA2"):
        super().__init__(text, parent)
        self.setCursor(Qt.PointingHandCursor)
        self.setStyleSheet(f"""
            QCheckBox {{
                color: #ECF0F1;
                font-family: 'Consolas';
                font-size: 11px;
                spacing: 8px;
            }}
            QCheckBox::indicator {{
                width: 14px;
                height: 14px;
                border: 1px solid #1E2A38;
                border-radius: 3px;
                background-color: #060A10;
            }}
            QCheckBox::indicator:unchecked:hover {{
                border-color: {active_color};
                box-shadow: 0px 0px 4px {active_color};
            }}
            QCheckBox::indicator:checked {{
                background-color: {active_color};
                border-color: {active_color};
                image: url(no_image_needed); /* Clear default */
            }}
            QCheckBox::indicator:checked:hover {{
                background-color: #FFFFFF;
                border-color: #FFFFFF;
            }}
        """)

class NeonSlider(QSlider):
    """Glow-accent slider bar."""
    def __init__(self, orientation=Qt.Horizontal, parent=None, active_color="#00FFA2"):
        super().__init__(orientation, parent)
        self.setCursor(Qt.PointingHandCursor)
        self.setStyleSheet(f"""
            QSlider::groove:horizontal {{
                border: 1px solid #1E2A38;
                height: 6px;
                background: #060A10;
                margin: 2px 0;
                border-radius: 3px;
            }}
            QSlider::handle:horizontal {{
                background: {active_color};
                border: 1px solid {active_color};
                width: 12px;
                height: 12px;
                margin: -3px 0;
                border-radius: 6px;
            }}
            QSlider::handle:horizontal:hover {{
                background: #FFFFFF;
                border-color: #FFFFFF;
            }}
        """)

class DiagnosticSensorCard(GlassPanel):
    """Premium visual sensor panel displaying dynamic status metrics."""
    def __init__(self, title, label_text, unit, parent=None, border_color="#00FFA2"):
        super().__init__(parent, border_color)
        self.unit = unit
        
        layout = QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        
        # Header mark
        self.lbl_title = QLabel(title.upper())
        self.lbl_title.setFont(QFont("Consolas", 8, QFont.Bold))
        self.lbl_title.setStyleSheet("color: #7F8C8D; border: none; background: transparent;")
        layout.addWidget(self.lbl_title)
        
        # Bottom content frame
        h_layout = QHBoxLayout()
        h_layout.setContentsMargins(0, 0, 0, 0)
        
        self.lbl_metric = QLabel("0")
        self.lbl_metric.setFont(QFont("Consolas", 18, QFont.Bold))
        self.lbl_metric.setStyleSheet("color: #FFFFFF; border: none; background: transparent;")
        h_layout.addWidget(self.lbl_metric)
        
        self.lbl_unit = QLabel(unit)
        self.lbl_unit.setFont(QFont("Consolas", 9, QFont.Bold))
        self.lbl_unit.setStyleSheet(f"color: {border_color}; border: none; background: transparent;")
        self.lbl_unit.setAlignment(Qt.AlignBottom | Qt.AlignLeft)
        h_layout.addWidget(self.lbl_unit)
        h_layout.addStretch()
        
        layout.addLayout(h_layout)
        
        self.progress_bar = NeonProgressBar(self, border_color)
        layout.addWidget(self.progress_bar)
        
    def update_val(self, val, text_override=None):
        """Updates internal displays."""
        self.progress_bar.setValue(int(max(0, min(100, val))))
        if text_override:
            self.lbl_metric.setText(text_override)
        else:
            self.lbl_metric.setText(str(int(val)))
