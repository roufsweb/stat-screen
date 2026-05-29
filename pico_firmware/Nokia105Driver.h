#ifndef NOKIA_105_DRIVER_H
#define NOKIA_105_DRIVER_H

#include "DisplayDriver.h"
#include "hardware/structs/sio.h"

class Nokia105Driver : public DisplayDriver {
private:
    int pinReset;
    int pinCS;
    int pinSDA;
    int pinSCK;

    // SPFD54124B display commands
    static const uint8_t SPLOUT  = 0x11;
    static const uint8_t COLMOD  = 0x3A;
    static const uint8_t MADCTL  = 0x36;
    static const uint8_t DISPON  = 0x29;
    static const uint8_t CASET   = 0x2A;
    static const uint8_t PASET   = 0x2B;
    static const uint8_t RAMWR   = 0x2C;

    inline void set_sck(bool val) {
        if (val) sio_hw->gpio_set = (1ul << pinSCK);
        else sio_hw->gpio_clr = (1ul << pinSCK);
    }
    inline void set_sda(bool val) {
        if (val) sio_hw->gpio_set = (1ul << pinSDA);
        else sio_hw->gpio_clr = (1ul << pinSDA);
    }
    inline void set_cs(bool val) {
        if (val) sio_hw->gpio_set = (1ul << pinCS);
        else sio_hw->gpio_clr = (1ul << pinCS);
    }

    #define NOKIA_CLOCK_DELAY() asm volatile("nop\n nop\n nop\n nop\n nop\n")

    void write_cmd(uint8_t cmd) {
        set_sck(0);
        NOKIA_CLOCK_DELAY();
        set_cs(0);
        NOKIA_CLOCK_DELAY();
        
        // 9th bit: 0 (Command)
        set_sda(0);
        NOKIA_CLOCK_DELAY();
        set_sck(1);
        NOKIA_CLOCK_DELAY();
        set_sck(0);
        NOKIA_CLOCK_DELAY();
        
        for (int mask = 0x80; mask > 0; mask >>= 1) {
            set_sda(cmd & mask);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
        }
        set_cs(1);
        NOKIA_CLOCK_DELAY();
    }

    void write_data(uint8_t data) {
        set_sck(0);
        NOKIA_CLOCK_DELAY();
        set_cs(0);
        NOKIA_CLOCK_DELAY();
        
        // 9th bit: 1 (Data)
        set_sda(1);
        NOKIA_CLOCK_DELAY();
        set_sck(1);
        NOKIA_CLOCK_DELAY();
        set_sck(0);
        NOKIA_CLOCK_DELAY();
        
        for (int mask = 0x80; mask > 0; mask >>= 1) {
            set_sda(data & mask);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
        }
        set_cs(1);
        NOKIA_CLOCK_DELAY();
    }

    void set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
        write_cmd(CASET);
        write_data(0x00);
        write_data(x0 + 2); // Column offset correction
        write_data(0x00);
        write_data(x1 + 2);
        
        write_cmd(PASET);
        write_data(0x00);
        write_data(y0);
        write_data(0x00);
        write_data(y1);
        
        write_cmd(RAMWR);
    }

public:
    Nokia105Driver(int pin_reset = 16, int pin_cs = 17, int pin_sda = 19, int pin_sck = 18) 
        : DisplayDriver(128, 128), pinReset(pin_reset), pinCS(pin_cs), pinSDA(pin_sda), pinSCK(pin_sck) {}

    void init() override {
        pinMode(pinReset, OUTPUT);
        pinMode(pinCS, OUTPUT);
        pinMode(pinSDA, OUTPUT);
        pinMode(pinSCK, OUTPUT);

        set_cs(1);
        set_sck(0);
        set_sda(0);

        // Reset controller
        digitalWrite(pinReset, LOW);
        delay(50);
        digitalWrite(pinReset, HIGH);
        delay(50);
        
        // Extended Unlock
        write_cmd(0xC1);
        write_data(0xFF);
        write_data(0x83);
        write_data(0x40);
        
        // Sleep Out
        write_cmd(0x11);
        delay(150);
        
        // Timing Control
        write_cmd(0xCA);
        write_data(0x70);
        write_data(0x00);
        write_data(0xD9);
        
        // RGB Signal Timing
        write_cmd(0xB0);
        write_data(0x01);
        write_data(0x11);
        
        // Drive Ability
        write_cmd(0xC9);
        write_data(0x90);
        write_data(0x49);
        write_data(0x10);
        write_data(0x28);
        write_data(0x28);
        write_data(0x10);
        write_data(0x00);
        write_data(0x06);
        delay(20);
        
        // Positive Gamma
        write_cmd(0xC2);
        write_data(0x60);
        write_data(0x71);
        write_data(0x01);
        write_data(0x0E);
        write_data(0x05);
        write_data(0x02);
        write_data(0x09);
        write_data(0x31);
        write_data(0x0A);
        
        // Negative Gamma
        write_cmd(0xC3);
        write_data(0x67);
        write_data(0x30);
        write_data(0x61);
        write_data(0x17);
        write_data(0x48);
        write_data(0x07);
        write_data(0x05);
        write_data(0x33);
        delay(10);
        
        // Power Control 5 & 4
        write_cmd(0xB5);
        write_data(0x35);
        write_data(0x20);
        write_data(0x45);
        
        write_cmd(0xB4);
        write_data(0x33);
        write_data(0x25);
        write_data(0x4C);
        delay(10);
        
        // Color Mode 16-bit
        write_cmd(COLMOD);
        write_data(0x05);
        
        // BGR Mapping
        write_cmd(MADCTL);
        write_data(0x08);
        
        // Inversion ON
        write_cmd(0x21);
        
        // Clear panel RAM (128x160)
        clear_driver_ram();
        
        // Display ON
        write_cmd(DISPON);
        delay(10);
    }

    void clear_driver_ram() {
        write_cmd(CASET);
        write_data(0x00);
        write_data(2);
        write_data(0x00);
        write_data(127 + 2);
        
        write_cmd(PASET);
        write_data(0x00);
        write_data(0);
        write_data(0x00);
        write_data(159);
        
        write_cmd(RAMWR);
        
        set_cs(0);
        NOKIA_CLOCK_DELAY();
        for (int i = 0; i < 128 * 160; i++) {
            // Send 16 bits of black (0x0000)
            for (int b = 0; b < 2; b++) {
                set_sck(0);
                NOKIA_CLOCK_DELAY();
                set_sda(1); // Data
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
                for (int mask = 0x80; mask > 0; mask >>= 1) {
                    set_sda(0);
                    NOKIA_CLOCK_DELAY();
                    set_sck(1);
                    NOKIA_CLOCK_DELAY();
                    set_sck(0);
                    NOKIA_CLOCK_DELAY();
                }
            }
        }
        set_cs(1);
        NOKIA_CLOCK_DELAY();
    }

    void clear(uint16_t color) override {
        set_window(0, 0, 127, 127);
        set_cs(0);
        NOKIA_CLOCK_DELAY();
        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;
        for (int i = 0; i < 128 * 128; i++) {
            // High byte
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(hi & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }
            // Low byte
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(lo & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }
        }
        set_cs(1);
        NOKIA_CLOCK_DELAY();
    }

    void drawPixel(int x, int y, uint16_t color) override {
        if (x >= 0 && x < 128 && y >= 0 && y < 128) {
            set_window(x, y, x, y);
            set_cs(0);
            NOKIA_CLOCK_DELAY();
            uint8_t hi = color >> 8;
            uint8_t lo = color & 0xFF;
            // High byte
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(hi & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }
            // Low byte
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1);
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(lo & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }
            set_cs(1);
            NOKIA_CLOCK_DELAY();
        }
    }

    void pushFrame(const uint16_t* frameBuffer) override {
        set_window(0, 0, 127, 127);
        noInterrupts();
        set_cs(0);
        NOKIA_CLOCK_DELAY();

        for (int i = 0; i < 16384; ++i) {
            uint16_t pixel = frameBuffer[i];
            uint8_t hi = pixel >> 8;
            uint8_t lo = pixel & 0xFF;

            // --- Send High Byte ---
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1); // 9th bit = 1 (Data)
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(hi & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }

            // --- Send Low Byte ---
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            set_sda(1); // 9th bit = 1 (Data)
            NOKIA_CLOCK_DELAY();
            set_sck(1);
            NOKIA_CLOCK_DELAY();
            set_sck(0);
            NOKIA_CLOCK_DELAY();
            for (int mask = 0x80; mask > 0; mask >>= 1) {
                set_sda(lo & mask);
                NOKIA_CLOCK_DELAY();
                set_sck(1);
                NOKIA_CLOCK_DELAY();
                set_sck(0);
                NOKIA_CLOCK_DELAY();
            }
        }

        set_cs(1);
        NOKIA_CLOCK_DELAY();
        interrupts();
    }
};

#endif // NOKIA_105_DRIVER_H
