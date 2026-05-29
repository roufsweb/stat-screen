# 📱 ROUFESWEB Cyber Stat Screen - RP2040 Local Graphics Engine

An ultra-efficient, highly modular hardware diagnostic companion dashboard system. Engineered to offload 100% of graphics vector drawing and physics calculations to the **Raspberry Pi Pico (RP2040)**'s dual ARM Cortex-M0+ cores, achieving extremely smooth updates at **40 FPS** with **less than 0.1% host PC CPU utilization**!

Featuring a stunning, highly visual **'Water Tank' Sloshing Fluid Animation** with drifting rising bubbles and dynamic thermal color grading, optimized specifically for the **Nokia 105 (128x128)** screen.

---

## 🚀 Key Features

* **Host-Efficient Telemetry Protocol (MTP):** Streams a highly compressed **32-byte binary packet** once every 500ms over USB COM, dropping host diagnostic CPU overhead from ~10% (raw framebuffer streaming) to near-zero.
* **Dual-Core Execution Splits:** 
  * **Core 0:** Runs a non-blocking sliding-window parser to ingest serial streams and validate data via XOR checksums.
  * **Core 1:** Computes physics loops (fluid sloshing wave coordinates, bubble rising wiggles, popped bubbles) and renders double-buffered vector graphics at 40 FPS.
* **🫧 'Water Tank' Sloshing Liquid Engine:** Models telemetry percentages as colored coolant fluid wiggling inside a glass cylinder using trigonometric sine-wave surface columns:
  $$y_{\text{surface}}(x) = y_{\text{level}} + A \sin\left(\frac{x - x_{\text{left}}}{\text{width}} \cdot 2\pi + \text{wave\_phase}\right)$$
* **Bubbles Drift Mechanics:** Floating wiggling bubble vectors ($x = x_{\text{base}} + 1.5 \sin(y \cdot 0.15)$) rise through the fluid and pop naturally at the surface.
* **Double Side-by-Side System Tanks:** Renders two independent sloshing tanks side-by-side for RAM and Disk C: metrics.
* **62.5 MHz Hardware SPI / SIO Drivers:** Includes modular display interfaces. Built-in support for **ST7789** displays at 62.5MHz and **Nokia 105 SPFD54124B** bit-banged panels.
* **🖥️ Asynchronous PySide6 (Qt6) Linker:** A premium dark-theme host linker utilizing QThreads for sensor polling (psutil, nvidia-smi) and QPainter to draw a real-time LCD simulator.

---

## 📐 System Architecture

```mermaid
graph TD
    subgraph Host PC (Companion App)
        diag[QThread Sensors: psutil / WMI / nvidia-smi] -->|Python dict| stream[Protocol Stream: Pack 32-Byte MTP]
        stream -->|USB CDC Serial @ 115200| com[Virtual COM Port]
    end

    subgraph RP2040 (Raspberry Pi Pico Core)
        com -->|Sliding Magic Buffer| core0[Core 0: Serial Listener & Pack Parser]
        core0 -->|Thread-Safe Copy| state[Shared Global Telemetry State]
        
        state -.->|Read under lock| core1[Core 1: 40 FPS Local Rendering Loop]
        
        subgraph Core 1 Engine
            core1 -->|Wave & Bubble Physics| math[AnimationSystem: Easing & Wave Solver]
            math -->|Double Buffer drawing| draw[GraphicsEngine: Shapes & Fonts]
            draw -->|SPI / Bit-bang push| driver[DisplayDriver: ST7789 / Nokia 105]
        end
    end
```

---

## 📂 Repository Structure

* **`pico_firmware/`:** Standalone C++ Arduino-Pico project folder.
  * **[TelemetryData.h](pico_firmware/TelemetryData.h):** Binary packet struct and XOR CRC validation.
  * **[DisplayDriver.h](pico_firmware/DisplayDriver.h):** Abstract base display hardware interface.
  * **[ST7789Driver.h](pico_firmware/ST7789Driver.h):** 62.5MHz hardware SPI display driver.
  * **[Nokia105Driver.h](pico_firmware/Nokia105Driver.h):** Nokia 105 SPFD54124B 9-bit bit-banger SIO register driver.
  * **[GraphicsEngine.h](pico_firmware/GraphicsEngine.h):** Vector shapes, circle arcs, meniscus lights, and 5x7 font renderer.
  * **[AnimationSystem.h](pico_firmware/AnimationSystem.h):** Lerp, slide transition runners, wiggling bubble systems.
  * **[pico_firmware.ino](pico_firmware/pico_firmware.ino):** Main orchestration sketch. Configured for Nokia 105 by default.
* **`desktop_app/`:** Asynchronous Python companion dashboard app.
  * **[protocol.py](desktop_app/protocol.py):** Packs serial telemetry bytes and auto-scans COM lines.
  * **[diagnostics.py](desktop_app/diagnostics.py):** QThread diagnostics gathering (CPU, GPU, RAM, Disk, Network).
  * **[widgets.py](desktop_app/widgets.py):** Customized Qt6 glassmorphic panel modules and neon buttons.
  * **[simulator.py](desktop_app/simulator.py):** QPainter visual vector LCD preview simulator at 40 FPS.
  * **[main.py](desktop_app/main.py):** PySide6 main event loop and layout coordinator.

---

## 🛠️ Flashing & Deployment Guides

### 1. Uploading Pico Firmware
1. Open the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the **Raspberry Pi Pico** board core by Earle Philhower (Settings -> Boards Manager).
3. Open the file **`pico_firmware/pico_firmware.ino`** in the IDE.
4. Select **Raspberry Pi Pico** as your target board.
5. Hit **Upload**! (Or hold BOOTSEL, copy the generated `.uf2` file into the Pico USB folder).
6. *To switch to a ST7789 240x240 display later, simply uncomment `#define SELECT_ST7789` and comment out `#define SELECT_NOKIA105` at the top of the `.ino` file! All scales adapt automatically!*

### 2. Launching Desktop Linker
1. Open a terminal inside the desktop directory:
   ```bash
   cd desktop_app
   ```
2. Install libraries:
   ```bash
   pip install PySide6 psutil pyserial
   ```
3. Run:
   ```bash
   python main.py
   ```
4. Select your Pico COM port from the dropdown list and click **⚡ ACTIVATE LINK**. The preview simulator instantly starts wiggling and wails in sync with your system workload!

---

## 🎨 Premium Visual Palette & Thermal Grade

* **Obsidian Base:** `#03070C` carbon black background, `#0D1520` glassmorphic cards, and `#08101C` matrix grid lines.
* **Submerged Dropshadow:** Submerged status text overlays draw a **+1px black shadow** behind the white body text, ensuring 100% legibility over moving waves!
* **Active Thermal Coolants:**
  * **Cool/Safe (< 50%):** Turquoise/Green (`#00FFA2`).
  * **Moderate (50% - 80%):** Amber Gold (`#FFAA00`).
  * **Critical Load (>= 80%):** Warning Red (`#FF3344`).

---

## 📜 License
Developed under the **MIT License**. Created by Roufesweb Diagnostic Labs.
