// ========================================================
// 📱 ROUFESWEB DUAL-CORE HARDWARE DIAGNOSTIC SCREEN v3.1
// ========================================================
// Featuring: High-Fidelity 'Water Tank' Sloshing Fluid Animations
// Splits tasks across dual cores:
//   - Core 0: High-speed non-blocking serial telemetry parsing (USB CDC).
//   - Core 1: 40 FPS vector drawing, sloshing coolant physics, drifting bubbles, and card sliding.

#include <Arduino.h>
#include "TelemetryData.h"
#include "DisplayDriver.h"
#include "ST7789Driver.h"
#include "Nokia105Driver.h"
#include "GraphicsEngine.h"
#include "AnimationSystem.h"

// ========================================================
// ⚙️ SYSTEM SELECTION CONFIG
// ========================================================
//#define SELECT_ST7789 // Standard high-speed ST7789 IPS LCD (240x240)
#define SELECT_NOKIA105 // TARGET THE NOKIA 105 PANEL (128x128) BY DEFAULT!

#ifdef SELECT_ST7789
  #define SCREEN_WIDTH  240
  #define SCREEN_HEIGHT 240
#else
  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT 128
#endif

// ========================================================
// 🔌 HARDWARE WIRING SPECIFICATIONS
// ========================================================
#ifdef SELECT_ST7789
  const int PIN_RESET = 16;
  const int PIN_CS    = 17;
  const int PIN_SCK   = 18;
  const int PIN_SDA   = 19;
  const int PIN_DC    = 20;
#else
  const int PIN_RESET = 16;
  const int PIN_CS    = 17;
  const int PIN_SCK   = 18;
  const int PIN_SDA   = 19;
#endif

// ========================================================
// 🧠 GLOBAL SYSTEM INSTANCES
// ========================================================
DisplayDriver* driver = nullptr;
GraphicsEngine* gfx = nullptr;

TelemetryState live_state;
TelemetryState render_state;

// ========================================================
// 🎨 ANIMATION & INTERPOLATION STATE
// ========================================================
AnimatedValue anim_cpu(0.0f, 0.12f);
AnimatedValue anim_cpu_temp(35.0f, 0.12f);
AnimatedValue anim_gpu(0.0f, 0.12f);
AnimatedValue anim_gpu_temp(35.0f, 0.12f);
AnimatedValue anim_ram(0.0f, 0.12f);
AnimatedValue anim_disk(0.0f, 0.12f);
AnimatedValue anim_net_dl(0.0f, 0.15f);
AnimatedValue anim_net_ul(0.0f, 0.15f);

SlideTransition card_slider(0.12f);
RollingHistory<float, 40> net_history;

enum DashboardCard { CARD_CPU = 0, CARD_GPU = 1, CARD_SYS = 2 };
DashboardCard active_card = CARD_CPU;
DashboardCard previous_card = CARD_CPU;

uint32_t last_card_swap = 0;
bool card_sliding = false;

// Standby animated values
float standby_angle = 0.0f;
AnimatedValue standby_fade(0.0f, 0.08f);

// Wave animation offset phase
float wave_phase = 0.0f;

// ========================================================
// 🫧 DRIFTING BUBBLES PHYSICS ENGINE
// ========================================================
struct Bubble {
    float x;
    float y;
    float base_x;
    float speed;
    float size;
    bool active;

    void reset(int t_left, int t_right, int t_bottom) {
        int w = t_right - t_left - 8;
        base_x = t_left + 4 + (rand() % (w > 0 ? w : 1));
        x = base_x;
        y = t_bottom - 2;
        speed = 0.4f + (rand() % 10) * 0.12f;
        size = 1.0f + (rand() % 2); // 1px or 2px bubbles
        active = true;
    }

    void update(int t_left, int t_right, int t_bottom, int y_surface) {
        if (!active) {
            reset(t_left, t_right, t_bottom);
            return;
        }

        y -= speed;
        x = base_x + sinf(y * 0.15f) * 1.5f;

        // Clip boundary
        if (x < t_left + 2) x = t_left + 2;
        if (x > t_right - 2) x = t_right - 2;

        // Pop bubble if it reaches wave surface
        if (y <= y_surface) {
            active = false;
        }
    }
};

// Declares bubbles list for active diagnostic cards
Bubble cpu_bubbles[3];
Bubble gpu_bubbles[3];
Bubble ram_bubbles[2];
Bubble disk_bubbles[2];

// ========================================================
// 🚀 CORE 0: PROTOCOL & SERIAL LISTENING
// ========================================================
uint8_t serial_rx_buffer[64];
size_t rx_buffer_idx = 0;

void parse_serial_stream() {
    while (Serial.available() > 0) {
        uint8_t incoming = Serial.read();

        if (rx_buffer_idx == 0 && incoming != 0xAA) continue;
        if (rx_buffer_idx == 1 && incoming != 0x55) {
            rx_buffer_idx = 0;
            continue;
        }

        serial_rx_buffer[rx_buffer_idx++] = incoming;

        if (rx_buffer_idx >= 32) {
            TelemetryState temp;
            if (parse_telemetry_packet(serial_rx_buffer, 32, temp)) {
                noInterrupts();
                live_state = temp;
                interrupts();
            }
            rx_buffer_idx = 0;
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Seed random wiggles
    randomSeed(analogRead(0) + micros());
}

void loop() {
    parse_serial_stream();
    delay(1);
}

// ========================================================
// 🚀 CORE 1: GRAPHICS ENGINE & VECTOR RENDERER
// ========================================================
void setup1() {
#ifdef SELECT_ST7789
    driver = new ST7789Driver(SCREEN_WIDTH, SCREEN_HEIGHT, PIN_DC, PIN_CS, PIN_RESET, PIN_SDA, PIN_SCK);
#else
    driver = new Nokia105Driver(PIN_RESET, PIN_CS, PIN_SDA, PIN_SCK);
#endif
    driver->init();
    gfx = new GraphicsEngine(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize bubble positions
    for (int i = 0; i < 3; ++i) {
        cpu_bubbles[i].reset(34, 94, 100);
        gpu_bubbles[i].reset(34, 94, 100);
    }
    for (int i = 0; i < 2; ++i) {
        ram_bubbles[i].reset(12, 56, 86);
        disk_bubbles[i].reset(72, 116, 86);
    }
}

void draw_header(const char* title, uint16_t accent_color) {
    int line_y = (SCREEN_WIDTH == 240) ? 30 : 16;
    int spacing = (SCREEN_WIDTH == 240) ? 6 : 8;

    gfx->drawString(spacing, 4, title, 0x7F8C, 1);
    gfx->fillRect(spacing, line_y - 2, SCREEN_WIDTH - 2 * spacing, 1, GraphicsEngine::rgb(30, 42, 56));
}

// ========================================================
// 🫧 HIGH-PERFORMANCE SLOSHING WATER TANK DRAW UTILITY
// ========================================================
void draw_water_tank(int offset_x, int tx_left, int tx_right, int ty_top, int ty_bottom, 
                     float val, uint16_t accent_color, const char* label, Bubble* bubbles, int num_bubbles) {
    int w = tx_right - tx_left;
    int h = ty_bottom - ty_top;

    // 1. Calculate base liquid height
    float h_fluid = h * (val / 100.0f);
    int y_level = ty_bottom - (int)h_fluid;

    // 2. Draw metallic frame
    // Double lines for tech casing
    gfx->drawRect(offset_x + tx_left, ty_top, w, h, GraphicsEngine::rgb(60, 80, 100)); // Inner
    gfx->drawRect(offset_x + tx_left - 2, ty_top - 2, w + 4, h + 4, GraphicsEngine::rgb(100, 120, 140)); // Outer

    // 3. Draw sloshing waves (rasterizing vertical columns inside the glass boundaries)
    for (int col = tx_left + 1; col < tx_right; ++col) {
        float theta = ((float)(col - tx_left) / w) * 2.0f * M_PI + wave_phase;
        float dy = 3.0f * sinf(theta); // 3px amplitude
        int y_surf = y_level + (int)dy;

        // Clip constraints
        y_surf = max(ty_top + 1, min(ty_bottom - 1, y_surf));

        // Draw solid coolant vertical line
        gfx->fillRect(offset_x + col, y_surf, 1, ty_bottom - y_surf, accent_color);
        // Draw slightly brighter surface line to simulate meniscus light reflections
        gfx->drawPixel(offset_x + col, y_surf, 0xFFFF);
    }

    // 4. Update and Draw Drifting Bubbles inside the liquid columns
    for (int i = 0; i < num_bubbles; ++i) {
        // Find water surface level at this bubble's x position to test deactivation
        int col_idx = (int)bubbles[i].x - tx_left;
        float theta = ((float)col_idx / w) * 2.0f * M_PI + wave_phase;
        int y_surf_x = y_level + (int)(3.0f * sinf(theta));
        y_surf_x = max(ty_top + 1, min(ty_bottom - 1, y_surf_x));

        bubbles[i].update(tx_left, tx_right, ty_bottom, y_surf_x);

        // Render bubble only if submerged
        if (bubbles[i].y > y_surf_x && bubbles[i].y < ty_bottom - 1) {
            uint16_t b_color = GraphicsEngine::rgb(220, 245, 255);
            if (bubbles[i].size > 1.5f) {
                // 2x2 bubble
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x + 1, (int)bubbles[i].y, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y + 1, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x + 1, (int)bubbles[i].y + 1, b_color);
            } else {
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y, b_color);
            }
        }
    }

    // 5. Render High-Contrast Numeric overlay in center of the tank
    char buf[16];
    sprintf(buf, "%d%%", (int)val);
    int text_x = offset_x + tx_left + w / 2 - strlen(buf) * 3;
    int text_y = ty_top + h / 2 - 4;

    // Drop Shadow for absolute contrast against liquid surface
    gfx->drawString(text_x + 1, text_y + 1, buf, 0x0000); // Black shadow
    gfx->drawString(text_x, text_y, buf, 0xFFFF);        // White body

    // Submerged label
    int lbl_x = offset_x + tx_left + w / 2 - strlen(label) * 3;
    int lbl_y = ty_top + h / 2 + 6;
    gfx->drawString(lbl_x + 1, lbl_y + 1, label, 0x0000);
    gfx->drawString(lbl_x, lbl_y, label, GraphicsEngine::rgb(200, 220, 240));
}

void draw_cpu_card(int offset_x) {
    float load = anim_cpu.update();
    float temp = anim_cpu_temp.update();
    uint16_t accent = GraphicsEngine::rgb(0, 255, 162); // Cool Turquoise

    if (temp > 75.0f) accent = GraphicsEngine::rgb(255, 51, 68); // Hot Red
    else if (temp > 55.0f) accent = GraphicsEngine::rgb(255, 170, 0); // Warm Gold

    draw_header("CPU DIAGNOSTICS", accent);

    // Render sloshing water tank
    // Large centered tank: Left 34, Right 94, Top 28, Bottom 100
    draw_water_tank(offset_x, 34, 94, 28, 100, load, accent, "LOAD", cpu_bubbles, 3);

    // Temperature bar/pill at bottom
    char buf[16];
    gfx->fillRoundRect(offset_x + 8, 106, SCREEN_WIDTH - 16, 16, 3, GraphicsEngine::rgb(18, 31, 45));
    int fill_w = (int)((SCREEN_WIDTH - 16) * (temp / 100.0f));
    gfx->fillRoundRect(offset_x + 8, 106, min(SCREEN_WIDTH - 16, fill_w), 16, 3, accent);
    
    sprintf(buf, "TEMP: %d C", (int)temp);
    gfx->drawString(offset_x + 14, 110, buf, GraphicsEngine::rgb(6, 10, 16), 1);
}

void draw_gpu_card(int offset_x) {
    float load = anim_gpu.update();
    float temp = anim_gpu_temp.update();
    uint16_t accent = GraphicsEngine::rgb(0, 223, 255); // Cyber Cyan

    if (temp > 75.0f) accent = GraphicsEngine::rgb(255, 51, 68);
    else if (temp > 55.0f) accent = GraphicsEngine::rgb(255, 170, 0);

    draw_header("GPU DIAGNOSTICS", accent);

    // Large centered tank
    draw_water_tank(offset_x, 34, 94, 28, 100, load, accent, "UTIL", gpu_bubbles, 3);

    char buf[16];
    gfx->fillRoundRect(offset_x + 8, 106, SCREEN_WIDTH - 16, 16, 3, GraphicsEngine::rgb(18, 31, 45));
    int fill_w = (int)((SCREEN_WIDTH - 16) * (temp / 100.0f));
    gfx->fillRoundRect(offset_x + 8, 106, min(SCREEN_WIDTH - 16, fill_w), 16, 3, accent);
    
    sprintf(buf, "TEMP: %d C", (int)temp);
    gfx->drawString(offset_x + 14, 110, buf, GraphicsEngine::rgb(6, 10, 16), 1);
}

void draw_system_card(int offset_x) {
    float ram = anim_ram.update();
    float disk = anim_disk.update();
    float dl = anim_net_dl.update();
    float ul = anim_net_ul.update();
    
    uint16_t gold = GraphicsEngine::rgb(255, 170, 0);
    uint16_t turquoise = GraphicsEngine::rgb(0, 255, 162);
    
    draw_header("SYS DIAGNOSTICS", gold);

    // Double Side-by-Side fluid tanks to display both RAM and DISK together!
    // Left Tank (RAM): Left 12, Right 56, Top 26, Bottom 86
    draw_water_tank(offset_x, 12, 56, 26, 86, ram, turquoise, "RAM", ram_bubbles, 2);

    // Right Tank (DISK C:): Left 72, Right 116, Top 26, Bottom 86
    draw_water_tank(offset_x, 72, 116, 26, 86, disk, gold, "DISK", disk_bubbles, 2);

    // Network speeds displayed at bottom
    char buf[32];
    sprintf(buf, "D:%.1f", dl);
    gfx->drawString(offset_x + 8, 93, buf, GraphicsEngine::rgb(0, 223, 255), 1);
    
    sprintf(buf, "U:%.1f", ul);
    gfx->drawString(offset_x + 68, 93, buf, GraphicsEngine::rgb(255, 51, 68), 1);

    // Rolling wave micro chart at the very bottom
    int y = 104;
    int graph_h = 18;
    int graph_w = SCREEN_WIDTH - 16;
    float max_dl = net_history.getMaxValue(2.0f);
    int prev_px = 0, prev_py = 0;
    
    for (int i = 0; i < net_history.size(); ++i) {
        int px = offset_x + 8 + (int)((float)i / (net_history.size() - 1) * graph_w);
        float val = net_history.get(i);
        int py = y + graph_h - (int)(graph_h * (val / max_dl) * 0.8f);
        py = max(y, min(y + graph_h - 1, py));

        if (i > 0) {
            gfx->drawLine(prev_px, prev_py, px, py, GraphicsEngine::rgb(0, 223, 255));
        }
        prev_px = px;
        prev_py = py;
    }
}

void draw_standby_screen() {
    gfx->fillScreen(GraphicsEngine::rgb(3, 7, 12));

    for (int cy = 0; cy < SCREEN_HEIGHT; cy += 16) {
        gfx->fillRect(0, cy, SCREEN_WIDTH, 1, GraphicsEngine::rgb(8, 16, 28));
    }
    for (int cx = 0; cx < SCREEN_WIDTH; cx += 16) {
        gfx->fillRect(cx, 0, 1, SCREEN_HEIGHT, GraphicsEngine::rgb(8, 16, 28));
    }

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2 - 10;
    int r = 20;

    standby_angle += 0.04f;
    if (standby_angle >= 2 * M_PI) standby_angle -= 2 * M_PI;

    uint16_t neon_cyan = GraphicsEngine::rgb(0, 223, 255);
    uint16_t neon_green = GraphicsEngine::rgb(0, 255, 162);
    
    for (int i = 0; i < 4; ++i) {
        float angle = standby_angle + (i * M_PI / 2.0f);
        int dx = cosf(angle) * r;
        int dy = sinf(angle) * r;
        gfx->drawLine(cx, cy, cx + dx, cy + dy, (i % 2 == 0) ? neon_cyan : neon_green);
    }
    gfx->drawCircle(cx, cy, r + 4, GraphicsEngine::rgb(18, 31, 45));
    gfx->fillCircle(cx, cy, 3, 0xFFFF);

    const char* line1 = "ROUFESWEB OS";
    const char* line2 = "STANDBY MODE";
    const char* line3 = "WAITING FOR COM LINK";

    int text_y = SCREEN_HEIGHT - 35;
    gfx->drawString(cx - strlen(line1)*3, text_y - 12, line1, neon_green, 1);
    gfx->drawString(cx - strlen(line2)*3, text_y - 2, line2, 0xFFFF, 1);
    gfx->drawString(cx - strlen(line3)*3, text_y + 8, line3, 0x7F8C, 1);
}

void draw_live_dashboard() {
    gfx->fillScreen(GraphicsEngine::rgb(3, 7, 12));

    for (int cy = 0; cy < SCREEN_HEIGHT; cy += 16) {
        gfx->fillRect(0, cy, SCREEN_WIDTH, 1, GraphicsEngine::rgb(8, 16, 28));
    }
    for (int cx = 0; cx < SCREEN_WIDTH; cx += 16) {
        gfx->fillRect(cx, 0, 1, SCREEN_HEIGHT, GraphicsEngine::rgb(8, 16, 28));
    }

    if (card_sliding) {
        float offset = card_slider.getOffset();
        card_sliding = card_slider.update();

        int dir = (active_card > previous_card) ? 1 : -1;
        if (abs(active_card - previous_card) > 1) {
            dir = -dir;
        }

        int prev_x = (int)offset;
        int active_x = prev_x + dir * SCREEN_WIDTH;

        switch (previous_card) {
            case CARD_CPU: draw_cpu_card(prev_x); break;
            case CARD_GPU: draw_gpu_card(prev_x); break;
            case CARD_SYS: draw_system_card(prev_x); break;
        }

        switch (active_card) {
            case CARD_CPU: draw_cpu_card(active_x); break;
            case CARD_GPU: draw_gpu_card(active_x); break;
            case CARD_SYS: draw_system_card(active_x); break;
        }
    } else {
        switch (active_card) {
            case CARD_CPU: draw_cpu_card(0); break;
            case CARD_GPU: draw_gpu_card(0); break;
            case CARD_SYS: draw_system_card(0); break;
        }
    }
}

void update_animations() {
    noInterrupts();
    render_state = live_state;
    interrupts();

    // Increment wave phase angle
    wave_phase += 0.15f;
    if (wave_phase >= 2.0f * M_PI) wave_phase -= 2.0f * M_PI;

    anim_cpu.set(render_state.cpu_load);
    anim_cpu_temp.set(render_state.cpu_temp);
    anim_gpu.set(render_state.gpu_load);
    anim_gpu_temp.set(render_state.gpu_temp);
    anim_ram.set(render_state.ram_percent);
    anim_disk.set(render_state.disk_percent);
    anim_net_dl.set(render_state.net_dl_rate);
    anim_net_ul.set(render_state.net_ul_rate);

    static uint32_t last_history_push = 0;
    if (millis() - last_history_push > 250) {
        net_history.push(render_state.net_dl_rate);
        last_history_push = millis();
    }

    uint32_t now = millis();
    if (!render_state.online || (now - render_state.last_update > 2500)) {
        render_state.online = false;
        standby_fade.set(0.0f);
        return;
    }

    standby_fade.set(1.0f);

    uint32_t cycle_ms = render_state.cycle_sec * 1000;
    if (now - last_card_swap > cycle_ms) {
        bool cpu_active = (render_state.active_cards & 0x01);
        bool gpu_active = (render_state.active_cards & 0x02);
        bool sys_active = (render_state.active_cards & 0x04);

        if (cpu_active || gpu_active || sys_active) {
            previous_card = active_card;
            
            int next = (active_card + 1) % 3;
            for (int i = 0; i < 3; ++i) {
                if (next == CARD_CPU && cpu_active) { active_card = CARD_CPU; break; }
                if (next == CARD_GPU && gpu_active) { active_card = CARD_GPU; break; }
                if (next == CARD_SYS && sys_active) { active_card = CARD_SYS; break; }
                next = (next + 1) % 3;
            }

            if (active_card != previous_card) {
                int dir = (active_card > previous_card) ? 1 : -1;
                if (abs(active_card - previous_card) > 1) dir = -dir;

                card_slider.force(0.0f);
                card_slider.slideTo(-dir * SCREEN_WIDTH);
                card_sliding = true;
            }
        }
        last_card_swap = now;
    }
}

void loop1() {
    uint32_t start_time = millis();

    update_animations();

    if (render_state.online) {
        draw_live_dashboard();
    } else {
        draw_standby_screen();
    }

    driver->pushFrame(gfx->getFrameBuffer());

    uint32_t elapsed = millis() - start_time;
    if (elapsed < 25) {
        delay(25 - elapsed);
    }
}
