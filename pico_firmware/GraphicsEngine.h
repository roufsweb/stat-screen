#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include <Arduino.h>

// Elegant 5x7 font definition for compact, high-legibility telemetry displays (ASCII 0x20 to 0x7E)
static const uint8_t font5x7[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5f, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7f, 0x14, 0x7f, 0x14, // #
    0x24, 0x2a, 0x7f, 0x2a, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1c, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1c, 0x00, // )
    0x14, 0x08, 0x3e, 0x08, 0x14, // *
    0x08, 0x08, 0x3e, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3e, 0x51, 0x49, 0x45, 0x3e, // 0
    0x00, 0x42, 0x7f, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4b, 0x31, // 3
    0x18, 0x14, 0x12, 0x7f, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3c, 0x4a, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1e, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x08, 0x14, 0x22, 0x41, 0x00, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x00, 0x41, 0x22, 0x14, 0x08, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3e, // @
    0x7e, 0x11, 0x11, 0x11, 0x7e, // A
    0x7f, 0x49, 0x49, 0x49, 0x36, // B
    0x3e, 0x41, 0x41, 0x41, 0x22, // C
    0x7f, 0x41, 0x41, 0x22, 0x1c, // D
    0x7f, 0x49, 0x49, 0x49, 0x41, // E
    0x7f, 0x09, 0x09, 0x09, 0x01, // F
    0x3e, 0x41, 0x49, 0x49, 0x7a, // G
    0x7f, 0x08, 0x08, 0x08, 0x7f, // H
    0x00, 0x41, 0x7f, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3f, 0x01, // J
    0x7f, 0x08, 0x14, 0x22, 0x41, // K
    0x7f, 0x40, 0x40, 0x40, 0x40, // L
    0x7f, 0x02, 0x0c, 0x02, 0x7f, // M
    0x7f, 0x04, 0x08, 0x10, 0x7f, // N
    0x3e, 0x41, 0x41, 0x41, 0x3e, // O
    0x7f, 0x09, 0x09, 0x09, 0x06, // P
    0x3e, 0x41, 0x51, 0x21, 0x5e, // Q
    0x7f, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7f, 0x01, 0x01, // T
    0x3f, 0x40, 0x40, 0x40, 0x3f, // U
    0x1f, 0x20, 0x40, 0x20, 0x1f, // V
    0x3f, 0x40, 0x38, 0x40, 0x3f, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7f, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // \ 
    0x00, 0x41, 0x41, 0x7f, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7f, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7f, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7e, 0x09, 0x01, 0x02, // f
    0x0c, 0x52, 0x52, 0x52, 0x3e, // g
    0x7f, 0x08, 0x04, 0x04, 0x7f, // h
    0x00, 0x44, 0x7d, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3d, 0x00, // j
    0x7f, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7f, 0x40, 0x00, // l
    0x7c, 0x04, 0x18, 0x04, 0x7c, // m
    0x7c, 0x08, 0x04, 0x04, 0x7c, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7c, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7c, // q
    0x7c, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3f, 0x44, 0x40, 0x20, // t
    0x3c, 0x40, 0x40, 0x20, 0x7c, // u
    0x1c, 0x20, 0x40, 0x20, 0x1c, // v
    0x3c, 0x40, 0x30, 0x40, 0x3c, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0c, 0x50, 0x50, 0x50, 0x3c, // y
    0x44, 0x64, 0x54, 0x4c, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7f, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x02, 0x01, 0x02, 0x04, 0x02, // ~
};

class GraphicsEngine {
private:
    int width;
    int height;
    uint16_t* buffer;
    bool ownBuffer;

public:
    GraphicsEngine(int w, int h) : width(w), height(h), ownBuffer(true) {
        buffer = new uint16_t[width * height];
        fillScreen(0x0000);
    }

    GraphicsEngine(int w, int h, uint16_t* externalBuffer) : width(w), height(h), buffer(externalBuffer), ownBuffer(false) {}

    ~GraphicsEngine() {
        if (ownBuffer) {
            delete[] buffer;
        }
    }

    const uint16_t* getFrameBuffer() const { return buffer; }
    uint16_t* getWritableBuffer() { return buffer; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Color conversion helper: RGB888 to RGB565 (packed 16-bit)
    static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // Direct write to RAM Framebuffer
    inline void drawPixel(int x, int y, uint16_t color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            buffer[y * width + x] = color;
        }
    }

    void fillScreen(uint16_t color) {
        // Fast fill using memory operations
        // std::fill_n is highly optimized in standard libraries
        std::fill_n(buffer, width * height, color);
    }

    // Draw solid rectangles
    void fillRect(int x, int y, int w, int h, uint16_t color) {
        int x_start = max(0, x);
        int x_end = min(width, x + w);
        int y_start = max(0, y);
        int y_end = min(height, y + h);

        for (int cy = y_start; cy < y_end; ++cy) {
            uint16_t* row = &buffer[cy * width];
            for (int cx = x_start; cx < x_end; ++cx) {
                row[cx] = color;
            }
        }
    }

    // Draw rectangle outline
    void drawRect(int x, int y, int w, int h, uint16_t color) {
        fillRect(x, y, w, 1, color);
        fillRect(x, y + h - 1, w, 1, color);
        fillRect(x, y, 1, h, color);
        fillRect(x + w - 1, y, 1, h, color);
    }

    // Draw filled circles
    void fillCircle(int cx, int cy, int r, uint16_t color) {
        int x_start = max(0, cx - r);
        int x_end = min(width - 1, cx + r);
        int y_start = max(0, cy - r);
        int y_end = min(height - 1, cy + r);
        int r2 = r * r;

        for (int y = y_start; y <= y_end; ++y) {
            int dy = y - cy;
            int dy2 = dy * dy;
            uint16_t* row = &buffer[y * width];
            for (int x = x_start; x <= x_end; ++x) {
                int dx = x - cx;
                if (dx * dx + dy2 <= r2) {
                    row[x] = color;
                }
            }
        }
    }

    // Draw circle outline (Bresenham's Midpoint Circle Algorithm)
    void drawCircle(int cx, int cy, int r, uint16_t color) {
        int x = 0;
        int y = r;
        int d = 3 - 2 * r;

        auto drawSymmetricPixels = [&](int px, int py) {
            drawPixel(cx + px, cy + py, color);
            drawPixel(cx - px, cy + py, color);
            drawPixel(cx + px, cy - py, color);
            drawPixel(cx - px, cy - py, color);
            drawPixel(cx + py, cy + px, color);
            drawPixel(cx - py, cy + px, color);
            drawPixel(cx + py, cy - px, color);
            drawPixel(cx - py, cy - px, color);
        };

        drawSymmetricPixels(x, y);
        while (y >= x) {
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
            drawSymmetricPixels(x, y);
        }
    }

    // Draw rounded rectangle outline
    void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
        if (r <= 0) {
            drawRect(x, y, w, h, color);
            return;
        }
        
        // Draw straight lines
        fillRect(x + r, y, w - 2 * r, 1, color);         // Top edge
        fillRect(x + r, y + h - 1, w - 2 * r, 1, color); // Bottom edge
        fillRect(x, y + r, 1, h - 2 * r, color);         // Left edge
        fillRect(x + w - 1, y + r, 1, h - 2 * r, color); // Right edge

        // Draw four quadrant corners using midpoint circle stepping
        int cx[4] = { x + r, x + w - r - 1, x + r, x + w - r - 1 };
        int cy[4] = { y + r, y + r, y + h - r - 1, y + h - r - 1 };
        
        int px = 0;
        int py = r;
        int d = 3 - 2 * r;

        auto drawCorners = [&](int dx, int dy) {
            drawPixel(cx[0] - dx, cy[0] - dy, color); // Top-Left
            drawPixel(cx[1] + dx, cy[1] - dy, color); // Top-Right
            drawPixel(cx[2] - dx, cy[2] + dy, color); // Bottom-Left
            drawPixel(cx[3] + dx, cy[3] + dy, color); // Bottom-Right

            drawPixel(cx[0] - dy, cy[0] - dx, color);
            drawPixel(cx[1] + dy, cy[1] - dx, color);
            drawPixel(cx[2] - dy, cy[2] + dx, color);
            drawPixel(cx[3] + dy, cy[3] + dx, color);
        };

        drawCorners(px, py);
        while (py >= px) {
            px++;
            if (d > 0) {
                py--;
                d = d + 4 * (px - py) + 10;
            } else {
                d = d + 4 * px + 6;
            }
            drawCorners(px, py);
        }
    }

    // Draw filled rounded containers
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
        if (r <= 0) {
            fillRect(x, y, w, h, color);
            return;
        }

        // Draw central vertical column
        fillRect(x + r, y, w - 2 * r, h, color);

        // Draw left and right columns (excluding corners)
        fillRect(x, y + r, r, h - 2 * r, color);
        fillRect(x + w - r, y + r, r, h - 2 * r, color);

        // Draw four quadrant circular corners
        int cx[4] = { x + r, x + w - r - 1, x + r, x + w - r - 1 };
        int cy[4] = { y + r, y + r, y + h - r - 1, y + h - r - 1 };
        
        int px = 0;
        int py = r;
        int d = 3 - 2 * r;

        auto fillCornerRows = [&](int dx, int dy) {
            // Top corners horizontal spans
            fillRect(cx[0] - dx, cy[0] - dy, dx, 1, color);
            fillRect(cx[1], cy[1] - dy, dx, 1, color);
            fillRect(cx[0] - dy, cy[0] - dx, dy, 1, color);
            fillRect(cx[1], cy[1] - dx, dy, 1, color);

            // Bottom corners horizontal spans
            fillRect(cx[2] - dx, cy[2] + dy, dx, 1, color);
            fillRect(cx[3], cy[3] + dy, dx, 1, color);
            fillRect(cx[2] - dy, cy[2] + dx, dy, 1, color);
            fillRect(cx[3], cy[3] + dx, dy, 1, color);
        };

        fillCornerRows(px, py);
        while (py >= px) {
            px++;
            if (d > 0) {
                py--;
                d = d + 4 * (px - py) + 10;
            } else {
                d = d + 4 * px + 6;
            }
            fillCornerRows(px, py);
        }
    }

    // High-performance arc drawing algorithm. Iterates only over the arc's bounding box.
    // Angles are in degrees: 0° is right (3 o'clock), 90° is down (6 o'clock), etc.
    void drawArc(int cx, int cy, int r, float startAngle, float endAngle, int thickness, uint16_t color) {
        // Bound calculation
        int r_outer = r;
        int r_inner = r - thickness;
        int r_outer2 = r_outer * r_outer;
        int r_inner2 = r_inner * r_inner;

        int x_min = max(0, cx - r_outer);
        int x_max = min(width - 1, cx + r_outer);
        int y_min = max(0, cy - r_outer);
        int y_max = min(height - 1, cy + r_outer);

        // Normalize angles to 0..360 range
        auto normalizeAngle = [](float a) {
            while (a < 0.0f) a += 360.0f;
            while (a >= 360.0f) a -= 360.0f;
            return a;
        };

        float normStart = normalizeAngle(startAngle);
        float normEnd = normalizeAngle(endAngle);
        bool crossZero = normEnd < normStart;

        for (int y = y_min; y <= y_max; ++y) {
            int dy = y - cy;
            int dy2 = dy * dy;
            uint16_t* row = &buffer[y * width];
            for (int x = x_min; x <= x_max; ++x) {
                int dx = x - cx;
                int dist2 = dx * dx + dy2;

                if (dist2 >= r_inner2 && dist2 <= r_outer2) {
                    // Check angle of the current pixel using fast atan2
                    float angle = atan2f(dy, dx) * 180.0f / M_PI;
                    if (angle < 0.0f) angle += 360.0f;

                    bool inAngle = false;
                    if (crossZero) {
                        inAngle = (angle >= normStart || angle <= normEnd);
                    } else {
                        inAngle = (angle >= normStart && angle <= normEnd);
                    }

                    if (inAngle) {
                        row[x] = color;
                    }
                }
            }
        }
    }

    // Draws custom clean characters from our embedded 5x7 monospaced font
    void drawChar(int x, int y, char c, uint16_t color, int size = 1) {
        if (c < 32 || c > 126) return; // Non-printable fallback
        int fontIdx = (c - 32) * 5;

        for (int col = 0; col < 5; ++col) {
            uint8_t line = pgm_read_byte(&font5x7[fontIdx + col]);
            for (int row = 0; row < 8; ++row) {
                if (line & (1 << row)) {
                    if (size == 1) {
                        drawPixel(x + col, y + row, color);
                    } else {
                        fillRect(x + col * size, y + row * size, size, size, color);
                    }
                }
            }
        }
    }

    // String drawing helper supporting font scaling
    void drawString(int x, int y, const char* str, uint16_t color, int size = 1, int letterSpacing = 1) {
        int cx = x;
        int cy = y;
        while (*str) {
            char c = *str++;
            if (c == '\n') {
                cx = x;
                cy += 8 * size;
            } else {
                drawChar(cx, cy, c, color, size);
                cx += (5 + letterSpacing) * size;
                if (cx >= width) break; // Screen clip safety
            }
        }
    }

    // Draws standard horizontal/vertical lines rapidly
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
        // Fast paths for horizontal/vertical lines
        if (x0 == x1) {
            fillRect(x0, min(y0, y1), 1, abs(y1 - y0) + 1, color);
            return;
        }
        if (y0 == y1) {
            fillRect(min(x0, x1), y0, abs(x1 - x0) + 1, 1, color);
            return;
        }

        // Standard Bresenham's Line Algorithm
        int dx = abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
};

#endif // GRAPHICS_ENGINE_H
