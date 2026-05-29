#ifndef ST7789_DRIVER_H
#define ST7789_DRIVER_H

#include "DisplayDriver.h"
#include <SPI.h>

class ST7789Driver : public DisplayDriver {
private:
    int pinDC;
    int pinCS;
    int pinReset;
    int pinSDA;
    int pinSCK;
#ifdef ARDUINO_ARCH_MBED
    arduino::MbedSPI* spi; // Pointer to hardware SPI
#else
    SPIClassRP2040* spi; // Pointer to hardware SPI0 or SPI1
#endif


    void writeCommand(uint8_t cmd) {
        digitalWrite(pinDC, LOW);
        digitalWrite(pinCS, LOW);
        spi->transfer(cmd);
        digitalWrite(pinCS, HIGH);
    }

    void writeData(uint8_t data) {
        digitalWrite(pinDC, HIGH);
        digitalWrite(pinCS, LOW);
        spi->transfer(data);
        digitalWrite(pinCS, HIGH);
    }

    void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
        writeCommand(0x2A); // CASET (Column Address Set)
        digitalWrite(pinDC, HIGH);
        digitalWrite(pinCS, LOW);
        spi->transfer16(x0);
        spi->transfer16(x1);
        digitalWrite(pinCS, HIGH);

        writeCommand(0x2B); // PASET (Page/Row Address Set)
        digitalWrite(pinDC, HIGH);
        digitalWrite(pinCS, LOW);
        spi->transfer16(y0);
        spi->transfer16(y1);
        digitalWrite(pinCS, HIGH);

        writeCommand(0x2C); // RAMWR (RAM Write)
    }

public:
    // Defaults: GP20 (DC), GP17 (CS), GP16 (RESET), GP19 (SDA/MOSI), GP18 (SCK)
    ST7789Driver(int w = 240, int h = 240, int pin_dc = 20, int pin_cs = 17, int pin_reset = 16, int pin_sda = 19, int pin_sck = 18)
        : DisplayDriver(w, h), pinDC(pin_dc), pinCS(pin_cs), pinReset(pin_reset), pinSDA(pin_sda), pinSCK(pin_sck) {
        
#ifdef ARDUINO_ARCH_MBED
        spi = &SPI;
#else
        // Select appropriate SPI hardware port based on the MOSI pin
        if (pinSDA == 19 || pinSDA == 11 || pinSDA == 3) {
            spi = &SPI;   // Hardware SPI0
        } else {
            spi = &SPI1;  // Hardware SPI1
        }
#endif
    }

    void init() override {
        pinMode(pinDC, OUTPUT);
        pinMode(pinCS, OUTPUT);
        pinMode(pinReset, OUTPUT);
        digitalWrite(pinCS, HIGH);

        // Configure hardware SPI pins
#ifndef ARDUINO_ARCH_MBED
        spi->setTX(pinSDA);
        spi->setSCK(pinSCK);
#endif
        spi->begin();

        // Perform hardware reset
        digitalWrite(pinReset, LOW);
        delay(50);
        digitalWrite(pinReset, HIGH);
        delay(120);

        // Initialize display registers inside a high-speed SPI transaction
        spi->beginTransaction(SPISettings(62500000, MSBFIRST, SPI_MODE3));

        writeCommand(0x01); // Software Reset
        delay(150);

        writeCommand(0x11); // Sleep Out
        delay(120);

        writeCommand(0x3A); // Color Mode
        writeData(0x05);    // 16-bit Color (RGB 565 format)

        writeCommand(0x36); // MADCTL (Memory Access Control)
        // 0x00 for standard RGB, 0x08 for BGR panel routing.
        // We set BGR mode since many ST7789 panels have BGR filter layouts.
        writeData(0x08); 

        writeCommand(0x21); // Display Inversion ON (Required for ST7789 IPS panels)
        writeCommand(0x13); // Normal Display Mode ON

        clear(0x0000);      // Clear Panel Buffer to Black

        writeCommand(0x29); // Display ON
        delay(50);
        
        spi->endTransaction();
    }

    void clear(uint16_t color) override {
        spi->beginTransaction(SPISettings(62500000, MSBFIRST, SPI_MODE3));
        setWindow(0, 0, width - 1, height - 1);
        
        digitalWrite(pinDC, HIGH);
        digitalWrite(pinCS, LOW);
        
        // Push uniform solid color blocks
        for (int i = 0; i < width * height; ++i) {
            spi->transfer16(color);
        }
        
        digitalWrite(pinCS, HIGH);
        spi->endTransaction();
    }

    void drawPixel(int x, int y, uint16_t color) override {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            spi->beginTransaction(SPISettings(62500000, MSBFIRST, SPI_MODE3));
            setWindow(x, y, x, y);
            
            digitalWrite(pinDC, HIGH);
            digitalWrite(pinCS, LOW);
            spi->transfer16(color);
            digitalWrite(pinCS, HIGH);
            
            spi->endTransaction();
        }
    }

    void pushFrame(const uint16_t* frameBuffer) override {
        spi->beginTransaction(SPISettings(62500000, MSBFIRST, SPI_MODE3));
        setWindow(0, 0, width - 1, height - 1);
        
        digitalWrite(pinDC, HIGH);
        digitalWrite(pinCS, LOW);

        // Hardware transfer with maximum clock rate
#ifdef ARDUINO_ARCH_MBED
        spi->transfer((void*)frameBuffer, width * height * 2);
#else
        spi->transfer((void*)frameBuffer, nullptr, width * height * 2);
#endif

        digitalWrite(pinCS, HIGH);
        spi->endTransaction();
    }
};

#endif // ST7789_DRIVER_H
