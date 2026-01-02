#pragma once
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

class BiquadFilter {
public:
    BiquadFilter() {
        reset();
    }

    void reset() {
        x1 = x2 = y1 = y2 = 0.f;
    }

    float process(float input) {
        float output = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        return output;
    }

    void setHighpass(float fc, float fs) {
        float w0 = 2.f * M_PI * fc / fs;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.f * 0.707f); // Q = 0.707 (Butterworth)

        float a0 = 1.f + alpha;
        b0 = (1.f + cosw0) / (2.f * a0);
        b1 = -(1.f + cosw0) / a0;
        b2 = (1.f + cosw0) / (2.f * a0);
        a1 = (-2.f * cosw0) / a0;
        a2 = (1.f - alpha) / a0;
    }

    void setLowpass(float fc, float fs) {
        float w0 = 2.f * M_PI * fc / fs;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.f * 0.707f); // Q = 0.707 (Butterworth)

        float a0 = 1.f + alpha;
        b0 = (1.f - cosw0) / (2.f * a0);
        b1 = (1.f - cosw0) / a0;
        b2 = (1.f - cosw0) / (2.f * a0);
        a1 = (-2.f * cosw0) / a0;
        a2 = (1.f - alpha) / a0;
    }

private:
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
};

class Delay {
public:
    Delay(double fs);
    void process(float *data, int numSamples, int delaySamples, float mix);
    void reset(double fs);
    float BL = 1;
    float FB = -0.4;
    float FF = 0;
    int M;
private:
    std::vector<float> delay_buf;
    int ridx;
    int widx;
    double sampleRate;
    BiquadFilter hpFilter;
    BiquadFilter lpFilter;
};
