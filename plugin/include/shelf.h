/*
  ==============================================================================

    shelf.h
    Created: 17 May 2024 12:43:30pm
    Author:  Anthony Hong

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <cstring>
#include <iostream>

class Shelf {
public:
    Shelf(float fs, float fc);
    void setValues(float val, const char *str);
    float applyHS(float x, int channel);
    float getGain() {return g.load();};
    void setSr(float val) {
        omega = 2*M_PI*f_cen/val;
    }
private:
    float omega;
    std::atomic<float> g;
    float x1[2];
    float x2[2];
    float y1[2];
    float y2[2];
    float Q;
    float f_cen;
};
