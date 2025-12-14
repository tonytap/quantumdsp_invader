/*
  ==============================================================================

    pn.cpp
    Created: 16 May 2024 6:02:49pm
    Author:  Anthony Hong

  ==============================================================================
*/

#include "pn.h"

PeakNotch::PeakNotch(float fs, float fc): fs(fs), fc(fc) {
    d = -1*cos(2*M_PI*fc/fs);
    g.store(0);
    xh1[0] = 0;
    xh2[0] = 0;
    xh2[0] = 0;
    xh2[1] = 0;
    fb = 30.0;
    if (fc == 140) {
        fb = 84.8484848485;
    } else if (fc == 800) {
        fb = 484.8484848485;
    } else if (fc == 5750) {
        fb = 3443.1137724551;
    }
    else if (fc == 5200) {
        fb = 764.7;
    }
    else if (fc == 120) {
        fb = 56.07476635514;
    }
    else if (fc == 200) {
        fb = 108.108108;
    }
    else if (fc == 116) {
        fb = 63.3879781421;
    }
}

float PeakNotch::applyPN(float x, int channel) {
    float V0 = pow(10, g.load()/20);
    float c;
    if (g.load() >= 0) {
        c = (tan(M_PI*fb/fs)-1)/(tan(M_PI*fb/fs)+1);
    } else {
        c = (tan(M_PI*fb/fs)-V0)/(tan(M_PI*fb/fs)+V0);
    }
    float xh = x-d*(1-c)*xh1[channel]+c*xh2[channel];
    float y1 = -c*xh+d*(1-c)*xh1[channel]+xh2[channel];
    xh2[channel] = xh1[channel];
    xh1[channel] = xh;
    return 0.5*(V0-1)*(x-y1)+x;
}

void PeakNotch::setValues(float val, const char *str) {
    if (strcmp(str, "g") == 0) {
        g.store(val);
    }
    if (strcmp(str, "q") == 0) {
        fb = fc/val;
    }
}

float PeakNotch::getGain() {
    return g.load();
}
