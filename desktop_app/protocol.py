import serial
import serial.tools.list_ports
import struct
import time
import logging

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")

class MicroTelemetryProtocol:
    @staticmethod
    def pack_telemetry(cpu_load, cpu_temp, cpu_freq, gpu_load, gpu_temp, gpu_vram, 
                        ram_percent, ram_used_mb, ram_total_mb, disk_percent, 
                        net_dl, net_ul, active_cards, cycle_sec, uptime_sec):
        """
        Packs host telemetry data into a 32-byte MTP binary structure.
        Format matching C++:
        < (little-endian)
        Offset  Type     Field
        0       uint8_t  magic1 (0xAA)
        1       uint8_t  magic2 (0x55)
        2       uint8_t  packet_type (0x01)
        3       uint8_t  cpu_load
        4       uint8_t  cpu_temp
        5-6     uint16_t cpu_freq
        7       uint8_t  gpu_load
        8       uint8_t  gpu_temp
        9       uint8_t  gpu_vram
        10      uint8_t  ram_percent
        11-12   uint16_t ram_used_mb
        13-14   uint16_t ram_total_mb
        15      uint8_t  disk_percent
        16-19   float    net_dl_rate
        20-23   float    net_ul_rate
        24      uint8_t  active_cards
        25      uint8_t  cycle_sec
        26-29   uint32_t uptime_sec
        30      uint8_t  reserved (0x00)
        31      uint8_t  checksum
        """
        # Pack 31 bytes first
        data_31 = struct.pack(
            "<BBBBBHBBBBHHBffBBIB",
            0xAA, 0x55, 0x01,
            int(max(0, min(100, cpu_load))),
            int(max(0, min(120, cpu_temp))),
            int(max(0, min(65535, cpu_freq))),
            int(max(0, min(100, gpu_load))),
            int(max(0, min(120, gpu_temp))),
            int(max(0, min(100, gpu_vram))),
            int(max(0, min(100, ram_percent))),
            int(max(0, min(65535, ram_used_mb))),
            int(max(0, min(65535, ram_total_mb))),
            int(max(0, min(100, disk_percent))),
            float(net_dl),
            float(net_ul),
            int(active_cards),
            int(max(1, min(10, cycle_sec))),
            int(uptime_sec),
            0x00  # Reserved
        )
        
        # Calculate XOR checksum of first 31 bytes
        checksum = 0
        for b in data_31:
            checksum ^= b
            
        # Append checksum as 32nd byte
        packet = data_31 + struct.pack("B", checksum)
        return packet

class SerialStreamManager:
    def __init__(self):
        self.serial_conn = None
        self.active_port = None
        
    @staticmethod
    def list_ports():
        """Scans the system for serial COM ports and highlights potential Pico devices."""
        ports = serial.tools.list_ports.comports()
        detected = []
        for p in ports:
            # Highlight Pico/RP2040 standard USB CDC VIDs (0x2E8A) or typical names
            is_pico = "2E8A" in str(p.hwid).upper() or "PICO" in p.description.upper() or "RP2040" in p.description.upper()
            detected.append({
                "port": p.device,
                "desc": p.description,
                "hwid": p.hwid,
                "is_pico": is_pico
            })
        return detected

    def connect(self, port_name, baudrate=115200):
        """Attempts connection to the specified COM port."""
        self.disconnect()
        try:
            # Standard USB CDC serial doesn't rely on baudrate internally, but 115200 is used as standard
            self.serial_conn = serial.Serial(port_name, baudrate=baudrate, timeout=1.0)
            self.active_port = port_name
            logging.info(f"Successfully bound to serial COM port: {port_name}")
            return True
        except Exception as e:
            logging.error(f"Failed to connect to COM port {port_name}: {e}")
            self.serial_conn = None
            self.active_port = None
            return False

    def disconnect(self):
        """Safely closes serial connection."""
        if self.serial_conn and self.serial_conn.is_open:
            try:
                self.serial_conn.close()
            except Exception as e:
                logging.error(f"Error while closing serial port: {e}")
        self.serial_conn = None
        self.active_port = None

    def is_connected(self):
        """Checks connection integrity."""
        return self.serial_conn is not None and self.serial_conn.is_open

    def send_packet(self, packet):
        """Pushes 32-byte packet onto the serial line."""
        if not self.is_connected():
            return False
        try:
            self.serial_conn.write(packet)
            self.serial_conn.flush()
            return True
        except Exception as e:
            logging.error(f"Failed to transmit packet: {e}")
            self.disconnect()  # Connection failed, drop it
            return False
