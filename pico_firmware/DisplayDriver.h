#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

class DisplayDriver {
protected:
    int width;
    int height;
public:
    DisplayDriver(int w, int h) : width(w), height(h) {}
    virtual ~DisplayDriver() {}

    virtual void init() = 0;
    virtual void clear(uint16_t color) = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;
    virtual void pushFrame(const uint16_t* frameBuffer) = 0;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

#endif // DISPLAY_DRIVER_H
