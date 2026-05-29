// ========================================================
// 📱 ROUFESWEB UNIFIED HARDWARE DIAGNOSTIC SCREEN v3.2
// ========================================================
// Featuring: High-Fidelity 'Water Tank' Sloshing Fluid Animations
// Optimized for universal RP2040 compatibility (Arduino Mbed core or Earle Philhower core).
// Consolidates operations into a high-speed unified single-core event loop.

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

enum DashboardCard { CARD_CPU = 0, CARD_GPU = 1, CARD_RAM = 2, CARD_DISK = 3 };
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
        size = 1.0f + (rand() % 2);
        active = true;
    }

    void update(int t_left, int t_right, int t_bottom, int y_surface) {
        if (!active) {
            reset(t_left, t_right, t_bottom);
            return;
        }

        y -= speed;
        x = base_x + sinf(y * 0.15f) * 1.5f;

        if (x < t_left + 2) x = t_left + 2;
        if (x > t_right - 2) x = t_right - 2;

        if (y <= y_surface) {
            active = false;
        }
    }
};

Bubble cpu_bubbles[3];
Bubble gpu_bubbles[3];
Bubble ram_bubbles[3];
Bubble disk_bubbles[3];

// ========================================================
// 🚀 PROTOCOL & SERIAL LISTENING
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
                live_state = temp;
            }
            rx_buffer_idx = 0;
        }
    }
}

void draw_top_status_bar(int offset_x) {
    // Subtle status bar overlay at y = 4 to 15
    const char* card_name = "CPU CARD";
    if (render_state.online) {
        switch (active_card) {
            case CARD_CPU: card_name = "CPU CARD"; break;
            case CARD_GPU: card_name = "GPU CARD"; break;
            case CARD_RAM: card_name = "RAM CARD"; break;
            case CARD_DISK: card_name = "DISK CARD"; break;
        }
    } else {
        card_name = "STANDBY";
    }

    gfx->drawString(offset_x + 8, 4, card_name, GraphicsEngine::rgb(0, 255, 162), 1);
    
    char status_txt[20];
    if (render_state.online) {
        uint32_t seconds = render_state.uptime_sec;
        uint32_t h = seconds / 3600;
        uint32_t m = (seconds % 3600) / 60;
        uint32_t s = seconds % 60;
        sprintf(status_txt, "%02lu:%02lu:%02lu", h, m, s);
    } else {
        strcpy(status_txt, "OFFLINE");
    }
    int txt_w = strlen(status_txt) * 6;
    gfx->drawString(offset_x + SCREEN_WIDTH - 8 - txt_w, 4, status_txt, GraphicsEngine::rgb(127, 140, 141), 1);
    
    gfx->drawLine(offset_x + 4, 15, offset_x + SCREEN_WIDTH - 4, 15, GraphicsEngine::rgb(30, 42, 56));
}


// ========================================================
// 🫧 HIGH-PERFORMANCE SLOSHING WATER TANK DRAW UTILITY
// ========================================================
void draw_water_tank(int offset_x, int tx_left, int tx_right, int ty_top, int ty_bottom, 
                     float val, uint16_t accent_color, const char* title, const char* details_lines[], int num_lines, Bubble* bubbles, int num_bubbles) {
    int w = tx_right - tx_left;
    int h = ty_bottom - ty_top;

    float h_fluid = h * (val / 100.0f);
    int y_level = ty_bottom - (int)h_fluid;

    // Outer glass cylinders
    gfx->drawRect(offset_x + tx_left, ty_top, w, h, GraphicsEngine::rgb(30, 42, 56));
    gfx->drawRect(offset_x + tx_left - 2, ty_top - 2, w + 4, h + 4, GraphicsEngine::rgb(13, 21, 32));

    // Sloshing wave surface columns
    for (int col = tx_left + 1; col < tx_right; ++col) {
        float theta = ((float)(col - tx_left) / w) * 2.0f * M_PI + wave_phase;
        float dy = 3.0f * sinf(theta);
        int y_surf = y_level + (int)dy;

        y_surf = max(ty_top + 1, min(ty_bottom - 1, y_surf));

        gfx->fillRect(offset_x + col, y_surf, 1, ty_bottom - y_surf, accent_color);
        gfx->drawPixel(offset_x + col, y_surf, 0xFFFF);
    }

    // Drifting wiggling bubble particles
    for (int i = 0; i < num_bubbles; ++i) {
        int col_idx = (int)bubbles[i].x - tx_left;
        float theta = ((float)col_idx / w) * 2.0f * M_PI + wave_phase;
        int y_surf_x = y_level + (int)(3.0f * sinf(theta));
        y_surf_x = max(ty_top + 1, min(ty_bottom - 1, y_surf_x));

        bubbles[i].update(tx_left, tx_right, ty_bottom, y_surf_x);

        if (bubbles[i].y > y_surf_x && bubbles[i].y < ty_bottom - 1) {
            uint16_t b_color = GraphicsEngine::rgb(220, 245, 255);
            if (bubbles[i].size > 1.5f) {
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x + 1, (int)bubbles[i].y, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y + 1, b_color);
                gfx->drawPixel(offset_x + (int)bubbles[i].x + 1, (int)bubbles[i].y + 1, b_color);
            } else {
                gfx->drawPixel(offset_x + (int)bubbles[i].x, (int)bubbles[i].y, b_color);
            }
        }
    }

    // Dynamic text-stacking and centering algorithm
    int title_h = 16;
    int detail_h = 10;
    int spacing = 4;
    int total_text_h = title_h + num_lines * detail_h + num_lines * spacing;

    int start_y = ty_top + (h - total_text_h) / 2;

    // Draw Title (double size = 2)
    int title_w = strlen(title) * 12; // 6 pixels per char. Double size = 12.
    int title_x = offset_x + tx_left + w / 2 - title_w / 2;
    // Outline
    gfx->drawString(title_x - 1, start_y, title, 0x0000, 2);
    gfx->drawString(title_x + 1, start_y, title, 0x0000, 2);
    gfx->drawString(title_x, start_y - 1, title, 0x0000, 2);
    gfx->drawString(title_x, start_y + 1, title, 0x0000, 2);
    // Title
    gfx->drawString(title_x, start_y, title, 0xFFFF, 2);

    // Draw detail lines
    int curr_y = start_y + title_h + spacing;
    for (int i = 0; i < num_lines; ++i) {
        int line_w = strlen(details_lines[i]) * 6; // 6 pixels per character at size 1
        int lx = offset_x + tx_left + w / 2 - line_w / 2;
        // Outline
        gfx->drawString(lx - 1, curr_y, details_lines[i], 0x0000, 1);
        gfx->drawString(lx + 1, curr_y, details_lines[i], 0x0000, 1);
        gfx->drawString(lx, curr_y - 1, details_lines[i], 0x0000, 1);
        gfx->drawString(lx, curr_y + 1, details_lines[i], 0x0000, 1);
        // Value
        gfx->drawString(lx, curr_y, details_lines[i], GraphicsEngine::rgb(200, 220, 240), 1);
        curr_y += detail_h + spacing;
    }

}

void draw_cpu_card(int offset_x) {
    float load = anim_cpu.update();
    float temp = anim_cpu_temp.update();
    uint16_t accent = GraphicsEngine::rgb(0, 255, 162);

    if (temp > 75.0f) accent = GraphicsEngine::rgb(255, 51, 68);
    else if (temp > 55.0f) accent = GraphicsEngine::rgb(255, 170, 0);

    const char* details[3];
    int num_lines = 0;

    char load_str[16];
    char temp_str[16];
    char clock_str[16];

    if (render_state.active_options & OPT_CPU_LOAD) {
        sprintf(load_str, "USAGE: %d%%", (int)load);
        details[num_lines++] = load_str;
    }
    if (render_state.active_options & OPT_CPU_TEMP) {
        sprintf(temp_str, "TEMP: %dC", (int)temp);
        details[num_lines++] = temp_str;
    }
    if (render_state.active_options & OPT_CPU_CLOCK) {
        sprintf(clock_str, "CLOCK: %.1fG", (float)render_state.cpu_freq / 1000.0f);
        details[num_lines++] = clock_str;
    }

    draw_water_tank(offset_x, 4, 124, 18, 124, load, accent, "CPU", details, num_lines, cpu_bubbles, 3);
}

void draw_gpu_card(int offset_x) {
    float load = anim_gpu.update();
    float temp = anim_gpu_temp.update();
    uint16_t accent = GraphicsEngine::rgb(0, 223, 255);

    if (temp > 75.0f) accent = GraphicsEngine::rgb(255, 51, 68);
    else if (temp > 55.0f) accent = GraphicsEngine::rgb(255, 170, 0);

    const char* details[3];
    int num_lines = 0;

    char load_str[16];
    char temp_str[16];
    char vram_str[16];

    if (render_state.active_options & OPT_GPU_LOAD) {
        sprintf(load_str, "UTIL: %d%%", (int)load);
        details[num_lines++] = load_str;
    }
    if (render_state.active_options & OPT_GPU_TEMP) {
        sprintf(temp_str, "TEMP: %dC", (int)temp);
        details[num_lines++] = temp_str;
    }
    if (render_state.active_options & OPT_GPU_VRAM) {
        sprintf(vram_str, "VRAM: %d%%", render_state.gpu_vram);
        details[num_lines++] = vram_str;
    }

    draw_water_tank(offset_x, 4, 124, 18, 124, load, accent, "GPU", details, num_lines, gpu_bubbles, 3);
}

void draw_ram_card(int offset_x) {
    float ram = anim_ram.update();
    uint16_t accent = GraphicsEngine::rgb(0, 255, 162);

    const char* details[4];
    int num_lines = 0;

    char load_str[16];
    char size_str[24];
    char dl_str[16];
    char ul_str[16];

    if (render_state.active_options & OPT_RAM_LOAD) {
        sprintf(load_str, "USED: %d%%", (int)ram);
        details[num_lines++] = load_str;
    }
    if (render_state.active_options & OPT_RAM_SIZE) {
        sprintf(size_str, "SIZE: %.1fG/%.1fG", (float)render_state.ram_used_mb / 1024.0f, (float)render_state.ram_total_mb / 1024.0f);
        details[num_lines++] = size_str;
    }
    if (render_state.active_cards & 0x40) {
        float dl = anim_net_dl.update();
        float ul = anim_net_ul.update();
        if (dl >= 1.0f) sprintf(dl_str, "DL: %.1fM", dl);
        else sprintf(dl_str, "DL: %.0fK", dl * 1024.0f);
        
        if (ul >= 1.0f) sprintf(ul_str, "UL: %.1fM", ul);
        else sprintf(ul_str, "UL: %.0fK", ul * 1024.0f);

        details[num_lines++] = dl_str;
        details[num_lines++] = ul_str;
    }

    draw_water_tank(offset_x, 4, 124, 18, 124, ram, accent, "RAM", details, num_lines, ram_bubbles, 3);
}

void draw_disk_card(int offset_x) {
    float disk = anim_disk.update();
    uint16_t accent = GraphicsEngine::rgb(255, 170, 0);

    const char* details[2];
    int num_lines = 0;

    char load_str[16];
    char free_str[16];

    if (render_state.active_cards & 0x10) {
        sprintf(load_str, "USED: %d%%", (int)disk);
        details[num_lines++] = load_str;
    }
    if (render_state.active_cards & 0x20) {
        sprintf(free_str, "FREE: %d%%", 100 - (int)disk);
        details[num_lines++] = free_str;
    }

    draw_water_tank(offset_x, 4, 124, 18, 124, disk, accent, "DISK", details, num_lines, disk_bubbles, 3);
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

    draw_top_status_bar(0);

    if (card_sliding) {
        float offset = card_slider.getOffset();
        card_sliding = card_slider.update();

        int dir = (active_card > previous_card) ? 1 : -1;
        if (abs(active_card - previous_card) > 1) {
            if ((previous_card == CARD_CPU && active_card == CARD_DISK) || (previous_card == CARD_DISK && active_card == CARD_CPU)) {
                dir = -dir;
            } else {
                dir = -dir;
            }
        }

        int prev_x = (int)offset;
        int active_x = prev_x + dir * SCREEN_WIDTH;

        switch (previous_card) {
            case CARD_CPU: draw_cpu_card(prev_x); break;
            case CARD_GPU: draw_gpu_card(prev_x); break;
            case CARD_RAM: draw_ram_card(prev_x); break;
            case CARD_DISK: draw_disk_card(prev_x); break;
        }

        switch (active_card) {
            case CARD_CPU: draw_cpu_card(active_x); break;
            case CARD_GPU: draw_gpu_card(active_x); break;
            case CARD_RAM: draw_ram_card(active_x); break;
            case CARD_DISK: draw_disk_card(active_x); break;
        }
    } else {
        switch (active_card) {
            case CARD_CPU: draw_cpu_card(0); break;
            case CARD_GPU: draw_gpu_card(0); break;
            case CARD_RAM: draw_ram_card(0); break;
            case CARD_DISK: draw_disk_card(0); break;
        }
    }
}

void update_animations() {
    render_state = live_state;

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
        bool ram_active = (render_state.active_cards & 0x04);
        bool disk_active = (render_state.active_cards & 0x08);

        if (cpu_active || gpu_active || ram_active || disk_active) {
            previous_card = active_card;
            
            int next = (active_card + 1) % 4;
            for (int i = 0; i < 4; ++i) {
                if (next == CARD_CPU && cpu_active) { active_card = CARD_CPU; break; }
                if (next == CARD_GPU && gpu_active) { active_card = CARD_GPU; break; }
                if (next == CARD_RAM && ram_active) { active_card = CARD_RAM; break; }
                if (next == CARD_DISK && disk_active) { active_card = CARD_DISK; break; }
                next = (next + 1) % 4;
            }

            if (active_card != previous_card) {
                int dir = (active_card > previous_card) ? 1 : -1;
                if (abs(active_card - previous_card) > 1) {
                    if ((previous_card == CARD_CPU && active_card == CARD_DISK) || (previous_card == CARD_DISK && active_card == CARD_CPU)) {
                        dir = -dir;
                    }
                }

                card_slider.force(0.0f);
                card_slider.slideTo(-dir * SCREEN_WIDTH);
                card_sliding = true;
            }
        }
        last_card_swap = now;
    }
}

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0) + micros());
    
#ifdef SELECT_ST7789
    driver = new ST7789Driver(SCREEN_WIDTH, SCREEN_HEIGHT, PIN_DC, PIN_CS, PIN_RESET, PIN_SDA, PIN_SCK);
#else
    driver = new Nokia105Driver(PIN_RESET, PIN_CS, PIN_SDA, PIN_SCK);
#endif
    driver->init();
    gfx = new GraphicsEngine(SCREEN_WIDTH, SCREEN_HEIGHT);

    for (int i = 0; i < 3; ++i) {
        cpu_bubbles[i].reset(4, 124, 124);
        gpu_bubbles[i].reset(4, 124, 124);
        ram_bubbles[i].reset(4, 124, 124);
        disk_bubbles[i].reset(4, 124, 124);
    }
}

void loop() {
    uint32_t start_time = millis();

    parse_serial_stream();
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
