/*
  ==============================================================================

    shelf.cpp
    Created: 17 May 2024 12:43:30pm
    Author:  Anthony Hong

  ==============================================================================
*/

#include "shelf.h"

Shelf::Shelf(float fs, float fc) {
    omega = 2*M_PI*fc/fs;
    f_cen = fc;
    x1[0] = 0;
    x1[1] = 0;
    x2[0] = 0;
    x2[1] = 0;
    y1[0] = 0;
    y1[1] = 0;
    y2[0] = 0;
    y2[1] = 0;
    Q = 5;
}

void Shelf::setValues(float val, const char *str) {
    if (strcmp(str, "g") == 0) {
        g.store(val);
    }
    if (strcmp(str, "q") == 0) {
        Q = val;
    }
}

float Shelf::applyHS(float x, int channel) {
    float A = pow(10, g.load()/40);
    float beta = sqrt(A)/Q;
    float b0 = A*(A+1+(A-1)*cos(omega)+beta*sin(omega));
    float b1 = -2*A*(A-1+(A+1)*cos(omega));
    float b2 = A*(A+1+(A-1)*cos(omega)-beta*sin(omega));
    float a0 = A+1-(A-1)*cos(omega)+beta*sin(omega);
    float a1 = 2*(A-1-(A+1)*cos(omega));
    float a2 = A+1-(A-1)*cos(omega)-beta*sin(omega);
    float y = (b0/a0)*x+(b1/a0)*x1[channel]+(b2/a0)*x2[channel]-(a1/a0)*y1[channel]-(a2/a0)*y2[channel];
    y2[channel] = y1[channel];
    y1[channel] = y;
    x2[channel] = x1[channel];
    x1[channel] = x;
    return y;
}
