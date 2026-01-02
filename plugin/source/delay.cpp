#include "delay.h"

Delay::Delay(double fs) {
    ridx = 0;
    widx = 0;
    M = (int)(3*fs);
    delay_buf.resize(M);
    sampleRate = fs;
    hpFilter.setHighpass(250.f, fs);
    lpFilter.setLowpass(8000.f, fs);
}

void Delay::process(float *data, int numSamples, int delaySamples, float mix) {
    for (int s = 0; s < numSamples; s++) {
        float dry = data[s];

        // Read from delay buffer and apply filters
        float delayedSample = delay_buf[ridx];
        delayedSample = hpFilter.process(delayedSample);
        delayedSample = lpFilter.process(delayedSample);

        float delayed = BL*delayedSample;
        float xh = delayedSample*FB+dry;
        data[s] = (1.f-mix)*dry+mix*delayed;
        delay_buf[widx] = xh;
        ridx = (ridx+1)%delaySamples;
        widx = (widx+1)%delaySamples;
    }
}

void Delay::reset(double fs) {
    ridx = 0;
    widx = 0;
    M = (int)(3*fs);
    delay_buf.resize(M);
    sampleRate = fs;
    hpFilter.reset();
    lpFilter.reset();
    hpFilter.setHighpass(250.f, fs);
    lpFilter.setLowpass(8000.f, fs);
}
