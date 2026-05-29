import sys
import logging
import os
import winreg
from PySide6.QtCore import Qt, Slot, QTimer
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QPushButton, QComboBox, QGroupBox, QGridLayout, QMessageBox, 
    QSpacerItem, QSizePolicy, QFrame, QScrollArea, QSystemTrayIcon, QMenu
)
from PySide6.QtGui import QFont, QColor, QIcon

# Import custom core modules
from protocol import SerialStreamManager, MicroTelemetryProtocol
from diagnostics import DiagnosticsThread
from widgets import GlassPanel, NeonCheckBox, NeonSlider, DiagnosticSensorCard
from simulator import VirtualLCDSimulator

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Roufesweb Cyber Stat Screen")
        
        # Load premium custom application icon
        icon_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon.png")
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))
            
        self.setGeometry(100, 100, 940, 580)
        self.setFixedSize(940, 580)
        
        # Deep space premium dark theme
        self.setStyleSheet("""
            QMainWindow {
                background-color: #03070C;
            }
            QLabel {
                color: #ECF0F1;
                font-family: 'Consolas';
            }
            QComboBox {
                background-color: #060A10;
                color: #00FFA2;
                border: 1px solid #1E2A38;
                border-radius: 4px;
                padding: 5px;
                font-family: 'Consolas';
                font-size: 11px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox QAbstractItemView {
                background-color: #060A10;
                color: #ECF0F1;
                selection-background-color: #0D1520;
                selection-color: #00FFA2;
                border: 1px solid #1E2A38;
            }
            QGroupBox {
                border: 1px solid #1E2A38;
                border-radius: 6px;
                margin-top: 12px;
                font-family: 'Consolas';
                font-weight: bold;
                font-size: 10px;
                color: #7F8C8D;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 3px 0 3px;
            }
        """)

        # Core logic initializations
        self.serial_manager = SerialStreamManager()
        self.diagnostics_thread = None
        self.active_cards_mask = 0x0F  # Bit0: CPU, Bit1: GPU, Bit2: RAM, Bit3: DISK
        self.active_options_mask = 0x7F  # All options ON by default
        
        # Main Layout Setup
        self.init_ui()
        
        # Set Nokia 105 Screen (128x128) as the default target index at startup
        self.combo_res.setCurrentIndex(1)
        
        # Initialize System Tray
        self.init_tray()
        
        # Windows Startup Registry Config
        self.check_startup_status()
        
        # Auto-Scan ports at startup
        self.refresh_com_ports()
        
        # Launch diagnostic scanner thread
        self.start_diagnostics()


    def init_ui(self):
        # Master widget
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        
        master_layout = QVBoxLayout(central_widget)
        master_layout.setContentsMargins(15, 0, 15, 15)
        master_layout.setSpacing(10)

        # ----------------------------------------------------
        # 1. FUTURISTIC NEON HEADER
        # ----------------------------------------------------
        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(0, 10, 0, 5)
        
        lbl_logo = QLabel("⚙️ ROUFESWEB DIAGNOSTIC ENGINE v3.0")
        lbl_logo.setFont(QFont("Consolas", 12, QFont.Bold))
        lbl_logo.setStyleSheet("color: #00FFA2; padding: 4px; background: transparent;")
        header_layout.addWidget(lbl_logo)
        
        header_layout.addStretch()
        
        self.lbl_uptime = QLabel("UPTIME: 00:00:00")
        self.lbl_uptime.setFont(QFont("Consolas", 9, QFont.Bold))
        self.lbl_uptime.setStyleSheet("color: #7F8C8D; background: transparent;")
        header_layout.addWidget(self.lbl_uptime)
        
        master_layout.addLayout(header_layout)
        
        # Bottom neon separating line
        sep = QFrame()
        sep.setFixedHeight(2)
        sep.setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00FFA2, stop:0.5 #00DFFF, stop:1 #FF9A00);")
        master_layout.addWidget(sep)

        # ----------------------------------------------------
        # 2. PANELS GRID (Left: Setup, Middle: Stats, Right: LCD Simulator)
        # ----------------------------------------------------
        panels_layout = QHBoxLayout()
        panels_layout.setSpacing(15)

        # --- LEFT SIDEBAR (CONNECTION & HARDWARE CONFIGS) ---
        sidebar = QVBoxLayout()
        sidebar.setSpacing(12)
        
        # Connection Panel
        conn_panel = GlassPanel(self, "#1E2A38")
        conn_layout = QVBoxLayout(conn_panel)
        conn_layout.setContentsMargins(12, 12, 12, 12)
        
        lbl_conn_title = QLabel("🔌 SERIAL CONNECTION")
        lbl_conn_title.setFont(QFont("Consolas", 9, QFont.Bold))
        lbl_conn_title.setStyleSheet("color: #00FFA2; border: none; background: transparent;")
        conn_layout.addWidget(lbl_conn_title)
        
        self.combo_ports = QComboBox()
        self.combo_ports.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        conn_layout.addWidget(self.combo_ports)
        
        h_conn_btns = QHBoxLayout()
        self.btn_refresh = QPushButton("🔄 SCAN")
        self.btn_refresh.setFont(QFont("Consolas", 8, QFont.Bold))
        self.btn_refresh.setCursor(Qt.PointingHandCursor)
        self.btn_refresh.setStyleSheet("""
            QPushButton {
                background-color: #060A10;
                color: #ECF0F1;
                border: 1px solid #1E2A38;
                border-radius: 4px;
                padding: 6px;
            }
            QPushButton:hover {
                border-color: #00DFFF;
                color: #00DFFF;
            }
        """)
        self.btn_refresh.clicked.connect(self.refresh_com_ports)
        h_conn_btns.addWidget(self.btn_refresh)
        
        self.btn_connect = QPushButton("⚡ ACTIVATE LINK")
        self.btn_connect.setFont(QFont("Consolas", 8, QFont.Bold))
        self.btn_connect.setCursor(Qt.PointingHandCursor)
        self.btn_connect.setStyleSheet("""
            QPushButton {
                background-color: #00FFA2;
                color: #03070C;
                border: 1px solid #00FFA2;
                border-radius: 4px;
                padding: 6px;
            }
            QPushButton:hover {
                background-color: #FFFFFF;
                border-color: #FFFFFF;
            }
        """)
        self.btn_connect.clicked.connect(self.toggle_serial_link)
        h_conn_btns.addWidget(self.btn_connect)
        conn_layout.addLayout(h_conn_btns)
        sidebar.addWidget(conn_panel)

        # Hardware display config panel
        hw_panel = GlassPanel(self, "#1E2A38")
        hw_outer_layout = QVBoxLayout(hw_panel)
        hw_outer_layout.setContentsMargins(12, 12, 6, 12)
        hw_outer_layout.setSpacing(6)
        
        lbl_hw_title = QLabel("⚙️ SCREEN CONFIGURATION")
        lbl_hw_title.setFont(QFont("Consolas", 9, QFont.Bold))
        lbl_hw_title.setStyleSheet("color: #FF9A00; border: none; background: transparent;")
        hw_outer_layout.addWidget(lbl_hw_title)
        
        # Scroll Area for settings items to prevent overflow and look clean
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setStyleSheet("""
            QScrollArea {
                border: none;
                background-color: transparent;
            }
            QScrollBar:vertical {
                border: none;
                background-color: #060A10;
                width: 6px;
                margin: 0px;
                border-radius: 3px;
            }
            QScrollBar::handle:vertical {
                background-color: #1E2A38;
                min-height: 20px;
                border-radius: 3px;
            }
            QScrollBar::handle:vertical:hover {
                background-color: #00FFA2;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                border: none;
                background: none;
            }
        """)
        
        scroll_content = QWidget()
        scroll_content.setStyleSheet("background-color: transparent; border: none;")
        hw_layout = QVBoxLayout(scroll_content)
        hw_layout.setContentsMargins(0, 0, 6, 0)
        hw_layout.setSpacing(8)
        
        # Display resolution selectors (Toggles physical/simulator configurations!)
        lbl_res = QLabel("LCD Panel Type:")
        lbl_res.setStyleSheet("border: none; background: transparent; color: #7F8C8D; font-size: 10px;")
        hw_layout.addWidget(lbl_res)
        
        self.combo_res = QComboBox()
        self.combo_res.addItem("ST7789 IPS LCD (240x240)", 240)
        self.combo_res.addItem("Nokia 105 Screen (128x128)", 128)
        self.combo_res.currentIndexChanged.connect(self.resolution_swapped)
        hw_layout.addWidget(self.combo_res)
        
        # Screen content toggles
        lbl_toggle = QLabel("Active Telemetry Cards:")
        lbl_toggle.setStyleSheet("border: none; background: transparent; color: #7F8C8D; font-size: 10px; margin-top: 8px;")
        hw_layout.addWidget(lbl_toggle)
        
        self.chk_cpu = NeonCheckBox("CPU Diagnostics", self, "#00FFA2")
        self.chk_cpu.setChecked(True)
        self.chk_cpu.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_cpu)
        
        self.chk_gpu = NeonCheckBox("GPU Utilization", self, "#00DFFF")
        self.chk_gpu.setChecked(True)
        self.chk_gpu.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_gpu)
        
        self.chk_ram = NeonCheckBox("RAM Allocation", self, "#FF9A00")
        self.chk_ram.setChecked(True)
        self.chk_ram.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_ram)

        self.chk_disk = NeonCheckBox("DISK Occupancy", self, "#FFAA00")
        self.chk_disk.setChecked(True)
        self.chk_disk.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_disk)

        # Granular Option Toggles
        lbl_options = QLabel("Display Data Items:")
        lbl_options.setStyleSheet("border: none; background: transparent; color: #7F8C8D; font-size: 10px; margin-top: 8px;")
        hw_layout.addWidget(lbl_options)

        self.chk_cpu_load = NeonCheckBox("CPU Load %", self, "#00FFA2")
        self.chk_cpu_load.setChecked(True)
        self.chk_cpu_load.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_cpu_load)

        self.chk_cpu_temp = NeonCheckBox("CPU Temp (°C)", self, "#00FFA2")
        self.chk_cpu_temp.setChecked(True)
        self.chk_cpu_temp.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_cpu_temp)

        self.chk_cpu_clock = NeonCheckBox("CPU Clock (GHz)", self, "#00FFA2")
        self.chk_cpu_clock.setChecked(True)
        self.chk_cpu_clock.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_cpu_clock)

        self.chk_gpu_load = NeonCheckBox("GPU Load %", self, "#00DFFF")
        self.chk_gpu_load.setChecked(True)
        self.chk_gpu_load.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_gpu_load)

        self.chk_gpu_temp = NeonCheckBox("GPU Temp (°C)", self, "#00DFFF")
        self.chk_gpu_temp.setChecked(True)
        self.chk_gpu_temp.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_gpu_temp)

        self.chk_gpu_vram = NeonCheckBox("GPU VRAM %", self, "#00DFFF")
        self.chk_gpu_vram.setChecked(True)
        self.chk_gpu_vram.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_gpu_vram)

        self.chk_ram_load = NeonCheckBox("RAM Usage %", self, "#FF9A00")
        self.chk_ram_load.setChecked(True)
        self.chk_ram_load.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_ram_load)

        self.chk_ram_size = NeonCheckBox("RAM Size (GB)", self, "#FF9A00")
        self.chk_ram_size.setChecked(True)
        self.chk_ram_size.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_ram_size)

        self.chk_net_speed = NeonCheckBox("Network Speed", self, "#FF9A00")
        self.chk_net_speed.setChecked(True)
        self.chk_net_speed.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_net_speed)

        self.chk_disk_load = NeonCheckBox("Disk Usage %", self, "#FFAA00")
        self.chk_disk_load.setChecked(True)
        self.chk_disk_load.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_disk_load)

        self.chk_disk_free = NeonCheckBox("Disk Free %", self, "#FFAA00")
        self.chk_disk_free.setChecked(True)
        self.chk_disk_free.stateChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.chk_disk_free)
        
        # Windows Startup checkbox
        lbl_startup = QLabel("System Startup Config:")
        lbl_startup.setStyleSheet("border: none; background: transparent; color: #7F8C8D; font-size: 10px; margin-top: 8px;")
        hw_layout.addWidget(lbl_startup)
        
        self.chk_startup = NeonCheckBox("Launch on Windows Startup", self, "#00FFA2")
        self.chk_startup.setChecked(False)
        self.chk_startup.stateChanged.connect(self.toggle_startup)
        hw_layout.addWidget(self.chk_startup)
        
        # Cycle rotation duration slider
        lbl_cycle = QLabel("Dashboard Rotation Loop (sec):")
        lbl_cycle.setStyleSheet("border: none; background: transparent; color: #7F8C8D; font-size: 10px; margin-top: 10px;")
        hw_layout.addWidget(lbl_cycle)
        
        self.slider_cycle = NeonSlider(Qt.Horizontal, self, "#FF9A00")
        self.slider_cycle.setRange(1, 10)
        self.slider_cycle.setValue(3)
        self.slider_cycle.valueChanged.connect(self.sync_active_cards)
        hw_layout.addWidget(self.slider_cycle)
        
        self.lbl_cycle_val = QLabel("Rotation Time: 3 seconds")
        self.lbl_cycle_val.setFont(QFont("Consolas", 8))
        self.lbl_cycle_val.setStyleSheet("color: #FF9A00; border: none; background: transparent;")
        self.lbl_cycle_val.setAlignment(Qt.AlignRight)
        hw_layout.addWidget(self.lbl_cycle_val)
        
        scroll.setWidget(scroll_content)
        hw_outer_layout.addWidget(scroll)
        
        sidebar.addWidget(hw_panel)

        sidebar.addStretch()
        panels_layout.addLayout(sidebar, 1)

        # --- CENTER OVERVIEW (HIGH-TECH DIAGNOSTIC METERS) ---
        center_layout = QVBoxLayout()
        center_layout.setSpacing(10)
        
        lbl_meters_title = QLabel("📊 HOST LIVE SYSTEM SENSOR METERS")
        lbl_meters_title.setFont(QFont("Consolas", 10, QFont.Bold))
        lbl_meters_title.setStyleSheet("color: #FFFFFF;")
        center_layout.addWidget(lbl_meters_title)
        
        grid_meters = QGridLayout()
        grid_meters.setSpacing(10)
        
        # Group cards beautifully
        self.card_cpu_load = DiagnosticSensorCard("CPU overall usage", "CPU Load", "%", self, "#00FFA2")
        self.card_cpu_temp = DiagnosticSensorCard("CPU temperature", "CPU Temp", "°C", self, "#00FFA2")
        self.card_gpu_load = DiagnosticSensorCard("GPU utilization", "GPU Load", "%", self, "#00DFFF")
        self.card_gpu_temp = DiagnosticSensorCard("GPU thermal index", "GPU Temp", "°C", self, "#00DFFF")
        self.card_ram_load = DiagnosticSensorCard("Memory allocation", "RAM Load", "%", self, "#FF9A00")
        self.card_disk_load = DiagnosticSensorCard("Disk space occupancy", "C: space", "%", self, "#FF9A00")
        
        grid_meters.addWidget(self.card_cpu_load, 0, 0)
        grid_meters.addWidget(self.card_cpu_temp, 0, 1)
        grid_meters.addWidget(self.card_gpu_load, 1, 0)
        grid_meters.addWidget(self.card_gpu_temp, 1, 1)
        grid_meters.addWidget(self.card_ram_load, 2, 0)
        grid_meters.addWidget(self.card_disk_load, 2, 1)
        
        center_layout.addLayout(grid_meters)
        
        # Live network display metrics
        net_panel = GlassPanel(self, "#1E2A38")
        net_layout = QHBoxLayout(net_panel)
        net_layout.setContentsMargins(15, 8, 15, 8)
        
        self.lbl_net_dl = QLabel("DOWNLOAD: 0.00 MB/s")
        self.lbl_net_dl.setFont(QFont("Consolas", 8, QFont.Bold))
        self.lbl_net_dl.setStyleSheet("color: #00DFFF; border: none; background: transparent;")
        net_layout.addWidget(self.lbl_net_dl)
        
        net_layout.addStretch()
        
        self.lbl_net_ul = QLabel("UPLOAD: 0.00 MB/s")
        self.lbl_net_ul.setFont(QFont("Consolas", 8, QFont.Bold))
        self.lbl_net_ul.setStyleSheet("color: #FF3344; border: none; background: transparent;")
        net_layout.addWidget(self.lbl_net_ul)
        
        center_layout.addWidget(net_panel)
        panels_layout.addLayout(center_layout, 2)

        # --- RIGHT SIDEBAR (HIGH-FIDELITY LCD SIMULATOR) ---
        sim_sidebar = QVBoxLayout()
        sim_sidebar.setSpacing(10)
        
        lbl_sim_title = QLabel("📱 DUAL-CORE SIMULATOR")
        lbl_sim_title.setFont(QFont("Consolas", 10, QFont.Bold))
        lbl_sim_title.setStyleSheet("color: #FFFFFF;")
        sim_sidebar.addWidget(lbl_sim_title)
        
        # Premium tech enclosure frame
        self.enclosure = QFrame()
        self.enclosure.setObjectName("Enclosure")
        self.enclosure.setStyleSheet("""
            QFrame#Enclosure {
                background-color: #15202F;
                border: 2px solid #1E2A38;
                border-radius: 8px;
                padding: 10px;
            }
        """)
        enclosure_layout = QVBoxLayout(self.enclosure)
        enclosure_layout.setContentsMargins(10, 10, 10, 10)
        enclosure_layout.setAlignment(Qt.AlignCenter)
        
        self.simulator = VirtualLCDSimulator(self)
        enclosure_layout.addWidget(self.simulator)
        
        lbl_brand = QLabel("--- ROUFESWEB CYBER TFT ---")
        lbl_brand.setFont(QFont("Consolas", 7, QFont.Bold))
        lbl_brand.setStyleSheet("color: #7F8C8D; margin-top: 5px;")
        lbl_brand.setAlignment(Qt.AlignCenter)
        enclosure_layout.addWidget(lbl_brand)
        
        sim_sidebar.addWidget(self.enclosure)
        
        # Diagnostic debug console panel
        debug_panel = GlassPanel(self, "#1E2A38")
        debug_layout = QVBoxLayout(debug_panel)
        debug_layout.setContentsMargins(10, 8, 10, 8)
        
        self.lbl_debug = QLabel("SYSTEM LINK STATUS: DISCONNECTED")
        self.lbl_debug.setFont(QFont("Consolas", 8, QFont.Bold))
        self.lbl_debug.setStyleSheet("color: #FF3344; border: none; background: transparent;")
        debug_layout.addWidget(self.lbl_debug)
        
        sim_sidebar.addWidget(debug_panel)
        sim_sidebar.addStretch()
        panels_layout.addLayout(sim_sidebar, 1.3)

        master_layout.addLayout(panels_layout)

    # ----------------------------------------------------
    # BUTTON TAPPING & DYNAMIC LOGIC SLOTS
    # ----------------------------------------------------
    def refresh_com_ports(self):
        """Scans the host system for active COM ports."""
        self.combo_ports.clear()
        detected = SerialStreamManager.list_ports()
        
        if not detected:
            self.combo_ports.addItem("No COM Ports Found", None)
            self.btn_connect.setEnabled(False)
            return
            
        self.btn_connect.setEnabled(True)
        for d in detected:
            label = f"{d['port']} - {d['desc']}"
            if d["is_pico"]:
                label += " (PICO)"
            self.combo_ports.addItem(label, d["port"])

    def toggle_serial_link(self):
        """Toggles real-time streaming state on COM line."""
        if not self.serial_manager.is_connected():
            port_name = self.combo_ports.currentData()
            if not port_name:
                QMessageBox.warning(self, "Warning", "No valid COM port selected!")
                return
                
            if self.serial_manager.connect(port_name):
                self.btn_connect.setText("🛑 DETACH LINK")
                self.btn_connect.setStyleSheet("""
                    QPushButton {
                        background-color: #FF3344;
                        color: #FFFFFF;
                        border: 1px solid #FF3344;
                        border-radius: 4px;
                        padding: 6px;
                    }
                    QPushButton:hover {
                        background-color: #FFFFFF;
                        border-color: #FFFFFF;
                        color: #FF3344;
                    }
                """)
                self.lbl_debug.setText("SYSTEM LINK STATUS: BOUND & STREAMING")
                self.lbl_debug.setStyleSheet("color: #00FFA2; border: none; background: transparent;")
                self.simulator.set_online(True)
            else:
                QMessageBox.critical(self, "Connection Failure", f"Could not bind to COM port {port_name}!")
        else:
            self.serial_manager.disconnect()
            self.btn_connect.setText("⚡ ACTIVATE LINK")
            self.btn_connect.setStyleSheet("""
                QPushButton {
                    background-color: #00FFA2;
                    color: #03070C;
                    border: 1px solid #00FFA2;
                    border-radius: 4px;
                    padding: 6px;
                }
                QPushButton:hover {
                    background-color: #FFFFFF;
                    border-color: #FFFFFF;
                }
            """)
            self.lbl_debug.setText("SYSTEM LINK STATUS: DISCONNECTED")
            self.lbl_debug.setStyleSheet("color: #FF3344; border: none; background: transparent;")
            self.simulator.set_online(False)

    def resolution_swapped(self):
        """Fired when the user selects a different target resolution dropdown."""
        res_px = self.combo_res.currentData()
        self.simulator.set_resolution(res_px)
        logging.info(f"Visual simulator rescaled to {res_px}x{res_px} viewport")

    def sync_active_cards(self):
        """Combines UI toggles into the active screens bitmask."""
        mask = 0
        if self.chk_cpu.isChecked(): mask |= 0x01
        if self.chk_gpu.isChecked(): mask |= 0x02
        if self.chk_ram.isChecked(): mask |= 0x04
        if self.chk_disk.isChecked(): mask |= 0x08
        
        # Disk options & net speed packed in upper bits of active_cards mask
        if self.chk_disk_load.isChecked(): mask |= 0x10
        if self.chk_disk_free.isChecked(): mask |= 0x20
        if self.chk_net_speed.isChecked(): mask |= 0x40
        
        self.active_cards_mask = mask

        options = 0
        if self.chk_cpu_load.isChecked(): options |= 0x01
        if self.chk_cpu_temp.isChecked(): options |= 0x02
        if self.chk_cpu_clock.isChecked(): options |= 0x04
        if self.chk_gpu_load.isChecked(): options |= 0x08
        if self.chk_gpu_temp.isChecked(): options |= 0x10
        if self.chk_gpu_vram.isChecked(): options |= 0x20
        if self.chk_ram_load.isChecked(): options |= 0x40
        if self.chk_ram_size.isChecked(): options |= 0x80

        self.active_options_mask = options
        cycle_sec = self.slider_cycle.value()
        self.lbl_cycle_val.setText(f"Rotation Time: {cycle_sec} seconds")
        
        # Sync configurations to the visual simulator
        self.simulator.set_config(mask, cycle_sec, options)


    def start_diagnostics(self):
        """Launches the background QThread diagnostics polling loop."""
        self.diagnostics_thread = DiagnosticsThread(interval_ms=500)
        self.diagnostics_thread.telemetry_updated.connect(self.on_telemetry_updated)
        self.diagnostics_thread.start()
        logging.info("Asynchronous background diagnostics thread activated")

    @Slot(dict)
    def on_telemetry_updated(self, telemetry):
        """Triggers every time background diagnostics gathers a telemetry packet (500ms)."""
        # 1. Update Host GUI Sensors
        self.card_cpu_load.update_val(telemetry["cpu_load"])
        self.card_cpu_temp.update_val(telemetry["cpu_temp"])
        self.card_gpu_load.update_val(telemetry["gpu_load"])
        self.card_gpu_temp.update_val(telemetry["gpu_temp"])
        self.card_ram_load.update_val(telemetry["ram_percent"], f"{telemetry['ram_used_mb']/1024.0:.1f}G")
        self.card_disk_load.update_val(telemetry["disk_percent"])
        
        self.lbl_net_dl.setText(f"DOWNLOAD: {telemetry['net_dl']:.2f} MB/s")
        self.lbl_net_ul.setText(f"UPLOAD: {telemetry['net_ul']:.2f} MB/s")
        
        # Formulate uptime clock format: HH:MM:SS
        up = telemetry["uptime"]
        hh = up // 3600
        mm = (up % 3600) // 60
        ss = up % 60
        self.lbl_uptime.setText(f"UPTIME: {hh:02d}:{mm:02d}:{ss:02d}")

        # 2. Update Virtual LCD Simulator
        self.simulator.update_telemetry(telemetry)

        # 3. Stream binary packed packet over active serial COM link
        if self.serial_manager.is_connected():
            packet = MicroTelemetryProtocol.pack_telemetry(
                cpu_load=telemetry["cpu_load"],
                cpu_temp=telemetry["cpu_temp"],
                cpu_freq=telemetry["cpu_freq"],
                gpu_load=telemetry["gpu_load"],
                gpu_temp=telemetry["gpu_temp"],
                gpu_vram=telemetry["gpu_vram"],
                ram_percent=telemetry["ram_percent"],
                ram_used_mb=telemetry["ram_used_mb"],
                ram_total_mb=telemetry["ram_total_mb"],
                disk_percent=telemetry["disk_percent"],
                net_dl=telemetry["net_dl"],
                net_ul=telemetry["net_ul"],
                active_cards=self.active_cards_mask,
                cycle_sec=self.slider_cycle.value(),
                uptime_sec=telemetry["uptime"],
                active_options=self.active_options_mask
            )
            self.serial_manager.send_packet(packet)

    def closeEvent(self, event):
        """Intercepts close and minimizes to tray if tray is active."""
        if hasattr(self, 'tray_icon') and self.tray_icon.isVisible():
            self.hide()
            self.tray_icon.showMessage(
                "Roufesweb Cyber Stat Screen",
                "Minimized to system tray. Still streaming telemetry!",
                QSystemTrayIcon.Information,
                2000
            )
            event.ignore()
        else:
            if self.diagnostics_thread:
                self.diagnostics_thread.stop()
            self.serial_manager.disconnect()
            super().closeEvent(event)

    def init_tray(self):
        """Initializes the Windows system tray icon and background running menu."""
        self.tray_icon = QSystemTrayIcon(self)
        icon_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "icon.png")
        if os.path.exists(icon_path):
            self.tray_icon.setIcon(QIcon(icon_path))
        else:
            self.tray_icon.setIcon(self.windowIcon())
            
        tray_menu = QMenu()
        
        show_action = tray_menu.addAction("🔒 Open Dashboard")
        show_action.triggered.connect(self.showNormal)
        
        hide_action = tray_menu.addAction("🙈 Minimize to Tray")
        hide_action.triggered.connect(self.hide)
        
        tray_menu.addSeparator()
        
        exit_action = tray_menu.addAction("🛑 Complete Exit")
        exit_action.triggered.connect(self.complete_exit)
        
        self.tray_icon.setContextMenu(tray_menu)
        self.tray_icon.setVisible(True)
        
        # Double clicking the tray icon restores window
        self.tray_icon.activated.connect(self.tray_activated)

    def tray_activated(self, reason):
        if reason == QSystemTrayIcon.DoubleClick:
            self.showNormal()

    def complete_exit(self):
        """Triggers a clean termination bypassing closeEvent tray minimization."""
        self.tray_icon.setVisible(False)
        if self.diagnostics_thread:
            self.diagnostics_thread.stop()
        self.serial_manager.disconnect()
        QApplication.quit()

    def check_startup_status(self):
        """Checks Windows Registry to see if startup shortcut is enabled."""
        key_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
        app_name = "RoufeswebCyberStatScreen"
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path, 0, winreg.KEY_READ)
            try:
                winreg.QueryValueEx(key, app_name)
                self.chk_startup.setChecked(True)
            except FileNotFoundError:
                self.chk_startup.setChecked(False)
            winreg.CloseKey(key)
        except Exception as e:
            logging.error(f"Error checking startup key: {e}")
            self.chk_startup.setChecked(False)

    def toggle_startup(self, state):
        """Creates/removes Windows registry startup entry dynamically."""
        key_path = r"Software\Microsoft\Windows\CurrentVersion\Run"
        app_name = "RoufeswebCyberStatScreen"
        
        # Get absolute path of running exe or script
        if getattr(sys, 'frozen', False):
            # Compiled pyinstaller exe
            exe_path = f'"{sys.executable}"'
        else:
            # Script run
            script_path = os.path.abspath(sys.argv[0])
            exe_path = f'"{sys.executable}" "{script_path}"'
            
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, key_path, 0, winreg.KEY_SET_VALUE)
            if state == Qt.Checked or state == 2:
                winreg.SetValueEx(key, app_name, 0, winreg.REG_SZ, exe_path)
                logging.info(f"Created Windows startup registry: {exe_path}")
            else:
                try:
                    winreg.DeleteValue(key, app_name)
                    logging.info("Deleted Windows startup registry key")
                except FileNotFoundError:
                    pass
            winreg.CloseKey(key)
        except Exception as e:
            logging.error(f"Failed to edit Windows startup registry: {e}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    # Apply crisp uniform font
    app.setFont(QFont("Consolas", 9))
    
    window = MainWindow()
    window.show()
    sys.exit(app.exec())
