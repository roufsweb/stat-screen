#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <Arduino.h>

// Easing math utilities
namespace Easing {
    inline float lerp(float start, float end, float alpha) {
        return start + (end - start) * alpha;
    }

    inline float easeOutQuad(float t) {
        return t * (2.0f - t);
    }

    inline float easeOutCubic(float t) {
        float f = (t - 1.0f);
        return f * f * f + 1.0f;
    }
}

// Smoothly dampens value updates using linear interpolation (lerp)
class AnimatedValue {
private:
    float current;
    float target;
    float speed; // Lerp alpha factor (0.0 to 1.0)

public:
    AnimatedValue(float start = 0.0f, float spd = 0.15f) : current(start), target(start), speed(spd) {}

    void set(float t) { target = t; }
    void force(float v) { current = v; target = v; }
    void setSpeed(float spd) { speed = spd; }

    float update() {
        current += (target - current) * speed;
        return current;
    }

    float get() const { return current; }
    float getTarget() const { return target; }
};

// Rolling array buffer to maintain historical telemetry points (e.g. graphs)
template <typename T, int N>
class RollingHistory {
private:
    T data[N];
    int count;

public:
    RollingHistory() : count(0) {
        for (int i = 0; i < N; ++i) {
            data[i] = T(0);
        }
    }

    void push(T val) {
        // Shift values to the left
        for (int i = 0; i < N - 1; ++i) {
            data[i] = data[i + 1];
        }
        data[N - 1] = val;
        if (count < N) count++;
    }

    T get(int idx) const {
        if (idx < 0 || idx >= N) return T(0);
        return data[idx];
    }

    int size() const { return N; }

    T getMaxValue(T fallback = T(1)) const {
        T m = data[0];
        for (int i = 1; i < N; ++i) {
            if (data[i] > m) m = data[i];
        }
        return m > T(0) ? m : fallback;
    }

    void clear() {
        for (int i = 0; i < N; ++i) {
            data[i] = T(0);
        }
        count = 0;
    }
};

// Multi-card slide transition layout animator
class SlideTransition {
private:
    float currentOffset;
    float targetOffset;
    float speed;

public:
    SlideTransition(float spd = 0.15f) : currentOffset(0.0f), targetOffset(0.0f), speed(spd) {}

    void slideTo(float target) { targetOffset = target; }
    void force(float offset) { currentOffset = offset; targetOffset = offset; }

    bool update() {
        currentOffset += (targetOffset - currentOffset) * speed;
        // Returns true if sliding is still in progress
        return abs(targetOffset - currentOffset) > 0.05f;
    }

    float getOffset() const { return currentOffset; }
    float getTarget() const { return targetOffset; }
};

#endif // ANIMATION_SYSTEM_H
