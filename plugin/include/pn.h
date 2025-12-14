/*
  ==============================================================================

    pn.h
    Created: 16 May 2024 6:02:49pm
    Author:  Anthony Hong

  ==============================================================================
*/

#pragma once
#include <cmath>
#include <cstring>
#include <iostream>

class PeakNotch {
public:
    PeakNotch(float fs, float fc);
    float applyPN(float x, int channel);
    void setValues(float val, const char *str);
    float getGain();
    void setSr(float val) {
        fs = val;
        d = -1*cos(2*M_PI*fc/fs);
    };
    
private:
    float fs;
    float d;
    std::atomic<float> g;
    float xh1[2];
    float xh2[2];
    float fb;
    float fc;
};
