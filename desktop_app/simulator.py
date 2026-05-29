import math
import random
from PySide6.QtCore import Qt, QTimer, QRectF, QPointF
from PySide6.QtWidgets import QWidget
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QFont, QLinearGradient, QPainterPath

class BubbleSim:
    """Drifting, wiggling bubble physics simulator matching Pico C++."""
    def __init__(self, t_left, t_right, t_bottom):
        self.x = 0.0
        self.y = 0.0
        self.base_x = 0.0
        self.speed = 0.0
        self.size = 1.0
        self.active = False
        self.reset(t_left, t_right, t_bottom)

    def reset(self, t_left, t_right, t_bottom):
        w = t_right - t_left - 8
        self.base_x = t_left + 4 + random.randint(0, max(1, w))
        self.x = float(self.base_x)
        self.y = float(t_bottom - 2)
        self.speed = 0.4 + random.randint(0, 10) * 0.12
        self.size = float(random.choice([1, 2]))
        self.active = True

    def update(self, t_left, t_right, t_bottom, y_surface):
        if not self.active:
            self.reset(t_left, t_right, t_bottom)
            return

        self.y -= self.speed
        self.x = self.base_x + math.sin(self.y * 0.15) * 1.5

        # Clip bounds
        if self.x < t_left + 2: self.x = float(t_left + 2)
        if self.x > t_right - 2: self.x = float(t_right - 2)

        # Pop bubble at wave surface
        if self.y <= y_surface:
            self.active = False

class VirtualLCDSimulator(QWidget):
    """
    State-of-the-art vector graphics simulator widget.
    Fully updated to model the 'Water Tank' sloshing fluid and drifting bubble
    animations on the Nokia 105 display (128x128) by default!
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        # Target Nokia 105 screen resolution by default (128x128)
        self.width_px = 128
        self.height_px = 128
        self.setFixedSize(128, 128)
        
        # Telemetry data cache
        self.data = {
            "cpu_load": 0, "cpu_temp": 40, "cpu_freq": 3200,
            "gpu_load": 0, "gpu_temp": 40, "gpu_vram": 0,
            "ram_percent": 0, "ram_used_mb": 0, "ram_total_mb": 16384,
            "disk_percent": 0, "net_dl": 0.0, "net_ul": 0.0
        }
        
        # Smooth animated variables
        self.anim_cpu = 0.0
        self.anim_cpu_temp = 40.0
        self.anim_gpu = 0.0
        self.anim_gpu_temp = 40.0
        self.anim_ram = 0.0
        self.anim_disk = 0.0
        self.anim_net_dl = 0.0
        self.anim_net_ul = 0.0
        
        # Rolling network history for graph logging
        self.net_history = [0.0] * 40
        
        # Transition slide values
        self.active_card = 0  # 0: CPU, 1: GPU, 2: SYSTEM
        self.prev_card = 0
        self.slide_offset = 0.0
        self.slide_target = 0.0
        self.is_sliding = False
        self.wave_phase = 0.0
        self.active_cards_mask = 0x0F
        self.cycle_sec = 3
        self.active_options_mask = 0x7F
        self.online = False
        self.standby_angle = 0.0


        # Instantiate wiggling bubbles lists matching Pico C++ (full screen)
        self.cpu_bubbles = [BubbleSim(4, 124, 124) for _ in range(3)]
        self.gpu_bubbles = [BubbleSim(4, 124, 124) for _ in range(3)]
        self.ram_bubbles = [BubbleSim(4, 124, 124) for _ in range(3)]
        self.disk_bubbles = [BubbleSim(4, 124, 124) for _ in range(3)]
        
        # High-speed render loop (40 FPS)
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_simulation)
        self.timer.start(25) # 25ms = 40 FPS

    def set_resolution(self, res):
        """Sets simulator size to match either ST7789 (240x240) or Nokia 105 (128x128)."""
        self.width_px = res
        self.height_px = res
        self.setFixedSize(res, res)
        self.update()

    def update_telemetry(self, telemetry):
        self.data = telemetry
        self.online = True

    def set_online(self, online):
        self.online = online

    def set_config(self, mask, cycle_sec, options=0x7F):
        self.active_cards_mask = mask
        self.cycle_sec = cycle_sec
        self.active_options_mask = options

    def update_simulation(self):
        # 1. Easing value interpolation (Lerp dampening)
        alpha = 0.12
        self.anim_cpu += (self.data["cpu_load"] - self.anim_cpu) * alpha
        self.anim_cpu_temp += (self.data["cpu_temp"] - self.anim_cpu_temp) * alpha
        
        self.anim_gpu += (self.data["gpu_load"] - self.anim_gpu) * alpha
        self.anim_gpu_temp += (self.data["gpu_temp"] - self.anim_gpu_temp) * alpha
        
        self.anim_ram += (self.data["ram_percent"] - self.anim_ram) * alpha
        self.anim_disk += (self.data["disk_percent"] - self.anim_disk) * alpha
        self.anim_net_dl += (self.data["net_dl"] - self.anim_net_dl) * alpha
        self.anim_net_ul += (self.data["net_ul"] - self.anim_net_ul) * alpha

        # Increment sloshing wave offset angle
        self.wave_phase += 0.15
        if self.wave_phase >= 2 * math.pi:
            self.wave_phase -= 2 * math.pi

        # 2. Network History logger (once in 10 frames)
        if hasattr(self, '_tick_counter'):
            self._tick_counter = (self._tick_counter + 1) % 10
        else:
            self._tick_counter = 0
            
        if self._tick_counter == 0:
            self.net_history.pop(0)
            self.net_history.append(self.anim_net_dl)

        # 3. Slide transition updates
        if self.is_sliding:
            self.slide_offset += (self.slide_target - self.slide_offset) * 0.12
            if abs(self.slide_target - self.slide_offset) < 0.1:
                self.slide_offset = self.slide_target
                self.is_sliding = False
        else:
            # Re-schedule card rotation swap
            if self.online:
                if not hasattr(self, '_last_swap_time'):
                    self._last_swap_time = QTimer.singleShot(self.cycle_sec * 1000, self.trigger_card_swap)

        self.standby_angle += 0.04
        if self.standby_angle >= 2 * math.pi:
            self.standby_angle = 0.0

        self.update()

    def trigger_card_swap(self):
        if not self.online or self.is_sliding:
            QTimer.singleShot(1000, self.trigger_card_swap)
            return

        cpu_active = bool(self.active_cards_mask & 0x01)
        gpu_active = bool(self.active_cards_mask & 0x02)
        ram_active = bool(self.active_cards_mask & 0x04)
        disk_active = bool(self.active_cards_mask & 0x08)

        if cpu_active or gpu_active or ram_active or disk_active:
            self.prev_card = self.active_card
            next_card = (self.active_card + 1) % 4
            
            for _ in range(4):
                if next_card == 0 and cpu_active: self.active_card = 0; break
                if next_card == 1 and gpu_active: self.active_card = 1; break
                if next_card == 2 and ram_active: self.active_card = 2; break
                if next_card == 3 and disk_active: self.active_card = 3; break
                next_card = (next_card + 1) % 4

            if self.active_card != self.prev_card:
                dir_val = 1 if self.active_card > self.prev_card else -1
                if abs(self.active_card - self.prev_card) > 1:
                    if (self.prev_card == 0 and self.active_card == 3) or (self.prev_card == 3 and self.active_card == 0):
                        dir_val = -dir_val

                self.slide_offset = 0.0
                self.slide_target = -dir_val * self.width_px
                self.is_sliding = True

        QTimer.singleShot(self.cycle_sec * 1000, self.trigger_card_swap)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, False) # Keep crisp retro look!
        
        # Tech enclosure background box
        painter.setPen(QPen(QColor("#15202F"), 1))
        painter.setBrush(QBrush(QColor("#03070C")))
        painter.drawRect(0, 0, self.width() - 1, self.height() - 1)

        # Tech grid background lines
        painter.setPen(QPen(QColor("#08101C"), 1))
        grid_step = 16 if self.width_px == 240 else 8
        for cy in range(0, self.height_px, grid_step):
            painter.drawLine(0, cy, self.width_px, cy)
        for cx in range(0, self.width_px, grid_step):
            painter.drawLine(cx, 0, cx, self.height_px)

        if not self.online:
            self.draw_standby(painter)
            return

        # Handle viewport sliding translations
        if self.is_sliding:
            dir_val = 1 if self.active_card > self.prev_card else -1
            if abs(self.active_card - self.prev_card) > 1:
                dir_val = -dir_val

            prev_x = int(self.slide_offset)
            active_x = prev_x + dir_val * self.width_px

            painter.save()
            self.draw_single_card(painter, self.prev_card, prev_x)
            painter.restore()

            painter.save()
            self.draw_single_card(painter, self.active_card, active_x)
            painter.restore()
        else:
            painter.save()
            self.draw_single_card(painter, self.active_card, 0)
            painter.restore()

    def draw_single_card(self, painter, card_idx, offset_x):
        self.draw_top_status_bar(painter, offset_x)
        if card_idx == 0:
            self.draw_cpu_card(painter, offset_x)
        elif card_idx == 1:
            self.draw_gpu_card(painter, offset_x)
        elif card_idx == 2:
            self.draw_ram_card(painter, offset_x)
        elif card_idx == 3:
            self.draw_disk_card(painter, offset_x)

    def draw_top_status_bar(self, painter, offset_x):
        # Subtle status bar overlay at y = 4 to 15
        painter.setFont(QFont("Consolas", 6, QFont.Bold))
        painter.setPen(QColor("#00FFA2"))
        
        card_name = "CPU CARD"
        if self.online:
            if self.active_card == 0: card_name = "CPU CARD"
            elif self.active_card == 1: card_name = "GPU CARD"
            elif self.active_card == 2: card_name = "RAM CARD"
            elif self.active_card == 3: card_name = "DISK CARD"
        else:
            card_name = "STANDBY"
            
        painter.drawText(offset_x + 8, 12, card_name)
        
        painter.setPen(QColor("#7F8C8D"))
        if self.online:
            up = self.data.get("uptime", 0)
            hh = up // 3600
            mm = (up % 3600) // 60
            ss = up % 60
            status_txt = f"{hh:02d}:{mm:02d}:{ss:02d}"
        else:
            status_txt = "OFFLINE"
        painter.drawText(offset_x + self.width_px - 8 - len(status_txt)*4, 12, status_txt)
        
        painter.setPen(QPen(QColor("#1E2A38"), 1))
        painter.drawLine(offset_x + 4, 15, offset_x + self.width_px - 4, 15)


    # ========================================================
    # 🫧 HIGH-PERFORMANCE SLOSHING WATER TANK DRAW WIDGET
    # ========================================================
    def draw_water_tank(self, painter, offset_x, tx_left, tx_right, ty_top, ty_bottom, 
                        val, color, title, details_lines, bubbles_list):
        w = tx_right - tx_left
        h = ty_bottom - ty_top

        # 1. Calculate base liquid level
        h_fluid = h * (val / 100.0)
        y_level = ty_bottom - h_fluid

        # 2. Draw outer tech frames (glass cylinder)
        painter.setPen(QPen(QColor("#1E2A38"), 1))
        painter.setBrush(Qt.NoBrush)
        painter.drawRect(offset_x + tx_left, ty_top, w, h)
        
        painter.setPen(QPen(QColor("#0D1520"), 1))
        painter.drawRect(offset_x + tx_left - 2, ty_top - 2, w + 4, h + 4)

        # 3. Rasterize wave columns matching Pico C++
        painter.setPen(QPen(color, 1))
        for col in range(tx_left + 1, tx_right):
            theta = ((col - tx_left) / w) * 2.0 * math.pi + self.wave_phase
            dy = 3.0 * math.sin(theta)
            y_surf = y_level + dy
            
            # Clip constraint
            y_surf = max(ty_top + 1, min(ty_bottom - 1, y_surf))
            
            # Draw liquid column line
            painter.drawLine(offset_x + col, int(y_surf), offset_x + col, ty_bottom - 1)
            
            # Draw highlight meniscus pixel
            painter.setPen(QPen(QColor("#FFFFFF"), 1))
            painter.drawPoint(offset_x + col, int(y_surf))
            painter.setPen(QPen(color, 1)) # Reset pen

        # 4. Drifting bubble particles
        painter.setPen(QPen(QColor("#DCF5FF"), 1))
        for i in range(len(bubbles_list)):
            col_idx = int(bubbles_list[i].x) - tx_left
            theta = (col_idx / w) * 2.0 * math.pi + self.wave_phase
            y_surf_x = y_level + 3.0 * math.sin(theta)
            y_surf_x = max(ty_top + 1, min(ty_bottom - 1, y_surf_x))
            
            bubbles_list[i].update(tx_left, tx_right, ty_bottom, y_surf_x)
            
            if bubbles_list[i].y > y_surf_x and bubbles_list[i].y < ty_bottom - 1:
                bx = int(bubbles_list[i].x)
                by = int(bubbles_list[i].y)
                
                if bubbles_list[i].size > 1.5:
                    painter.drawPoint(offset_x + bx, by)
                    painter.drawPoint(offset_x + bx + 1, by)
                    painter.drawPoint(offset_x + bx, by + 1)
                    painter.drawPoint(offset_x + bx + 1, by + 1)
                else:
                    painter.drawPoint(offset_x + bx, by)

        # 5. Dynamic text-stacking and centering algorithm
        title_h = 16
        detail_h = 10
        spacing = 4
        N = len(details_lines)
        total_text_h = title_h + N * detail_h + (N) * spacing

        start_y = ty_top + (h - total_text_h) / 2

        # Draw Title
        painter.setFont(QFont("Consolas", 11, QFont.Bold))
        title_w = len(title) * 6
        title_x = offset_x + tx_left + w / 2 - title_w / 2
        # Outline
        painter.setPen(QColor("#000000"))
        painter.drawText(title_x - 1, start_y + 10, title)
        painter.drawText(title_x + 1, start_y + 10, title)
        painter.drawText(title_x, start_y + 9, title)
        painter.drawText(title_x, start_y + 11, title)
        # Value
        painter.setPen(QColor("#FFFFFF"))
        painter.drawText(title_x, start_y + 10, title)

        # Draw details lines
        painter.setFont(QFont("Consolas", 7, QFont.Bold))
        curr_y = start_y + title_h + spacing
        for line in details_lines:
            line_w = len(line) * 4.5
            lx = offset_x + tx_left + w / 2 - line_w / 2
            # Outline
            painter.setPen(QColor("#000000"))
            painter.drawText(lx - 1, curr_y + 8, line)
            painter.drawText(lx + 1, curr_y + 8, line)
            painter.drawText(lx, curr_y + 7, line)
            painter.drawText(lx, curr_y + 9, line)
            # Value
            painter.setPen(QColor("#C8DCF0"))
            painter.drawText(lx, curr_y + 8, line)
            curr_y += detail_h + spacing


    def draw_cpu_card(self, painter, offset_x):
        accent = QColor("#00FFA2")  # Turquoise
        temp = self.anim_cpu_temp
        if temp > 75.0: accent = QColor("#FF3344")
        elif temp > 55.0: accent = QColor("#FFAA00")

        details = []
        if self.active_options_mask & 0x01:
            details.append(f"USAGE: {int(self.anim_cpu)}%")
        if self.active_options_mask & 0x02:
            details.append(f"TEMP: {int(temp)}C")
        if self.active_options_mask & 0x04:
            details.append(f"CLOCK: {self.data['cpu_freq']/1000.0:.1f}GHz")

        self.draw_water_tank(painter, offset_x, 4, 124, 18, 124, self.anim_cpu, accent, "CPU", details, self.cpu_bubbles)

    def draw_gpu_card(self, painter, offset_x):
        accent = QColor("#00DFFF")  # Cyan
        temp = self.anim_gpu_temp
        if temp > 75.0: accent = QColor("#FF3344")
        elif temp > 55.0: accent = QColor("#FFAA00")

        details = []
        if self.active_options_mask & 0x08:
            details.append(f"UTIL: {int(self.anim_gpu)}%")
        if self.active_options_mask & 0x10:
            details.append(f"TEMP: {int(temp)}C")
        if self.active_options_mask & 0x20:
            details.append(f"VRAM: {self.data['gpu_vram']}%")

        self.draw_water_tank(painter, offset_x, 4, 124, 18, 124, self.anim_gpu, accent, "GPU", details, self.gpu_bubbles)

    def draw_ram_card(self, painter, offset_x):
        accent = QColor("#00FFA2")  # Turquoise
        
        details = []
        if self.active_options_mask & 0x40:
            details.append(f"USED: {int(self.anim_ram)}%")
        if self.active_options_mask & 0x80:
            details.append(f"SIZE: {self.data['ram_used_mb']/1024.0:.1f}G/{self.data['ram_total_mb']/1024.0:.1f}G")
        if hasattr(self, 'active_cards_mask') and (self.active_cards_mask & 0x40):
            dl_str = f"DL:{self.anim_net_dl:.1f}M" if self.anim_net_dl >= 1.0 else f"DL:{self.anim_net_dl*1024.0:.0f}K"
            ul_str = f"UL:{self.anim_net_ul:.1f}M" if self.anim_net_ul >= 1.0 else f"UL:{self.anim_net_ul*1024.0:.0f}K"
            details.append(dl_str)
            details.append(ul_str)

        self.draw_water_tank(painter, offset_x, 4, 124, 18, 124, self.anim_ram, accent, "RAM", details, self.ram_bubbles)

    def draw_disk_card(self, painter, offset_x):
        accent = QColor("#FF9A00")  # Gold
        
        details = []
        if hasattr(self, 'active_cards_mask') and (self.active_cards_mask & 0x10):
            details.append(f"USED: {int(self.anim_disk)}%")
        if hasattr(self, 'active_cards_mask') and (self.active_cards_mask & 0x20):
            details.append(f"FREE: {100 - int(self.anim_disk)}%")


        self.draw_water_tank(painter, offset_x, 4, 124, 18, 124, self.anim_disk, accent, "DISK", details, self.disk_bubbles)

    def draw_standby(self, painter):
        cx = self.width() / 2
        cy = self.height() / 2 - 10
        r = 20

        neon_cyan = QColor("#00DFFF")
        neon_green = QColor("#00FFA2")

        for i in range(4):
            angle = self.standby_angle + (i * math.pi / 2.0)
            dx = math.cos(angle) * r
            dy = math.sin(angle) * r
            
            painter.setPen(QPen(neon_cyan if i % 2 == 0 else neon_green, 1.5))
            painter.drawLine(QPointF(cx, cy), QPointF(cx + dx, cy + dy))

        painter.setPen(QPen(QColor("#121F2D"), 1.2))
        painter.setBrush(Qt.NoBrush)
        painter.drawEllipse(QRectF(cx - r - 4, cy - r - 4, 2*(r+4), 2*(r+4)))

        painter.setPen(Qt.NoPen)
        painter.setBrush(QBrush(QColor("#FFFFFF")))
        painter.drawEllipse(QPointF(cx, cy), 2, 2)

        line1 = "ROUFESWEB OS"
        line2 = "STANDBY MODE"
        line3 = "WAITING FOR COM LINK"

        y_text = self.height() - 35
        painter.setFont(QFont("Consolas", 7, QFont.Bold))
        painter.setPen(neon_green)
        painter.drawText(QRectF(0, y_text - 12, self.width_px, 15), Qt.AlignCenter, line1)
        
        painter.setFont(QFont("Consolas", 6))
        painter.setPen(QColor("#FFFFFF"))
        painter.drawText(QRectF(0, y_text - 2, self.width_px, 10), Qt.AlignCenter, line2)
        
        painter.setPen(QColor("#7F8C8D"))
        painter.drawText(QRectF(0, y_text + 6, self.width_px, 10), Qt.AlignCenter, line3)
