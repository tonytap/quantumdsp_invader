/*
  ==============================================================================

    reverb.h
    Created: 18 May 2024 12:33:00pm
    Author:  Anthony Hong

  ==============================================================================
*/

#pragma once

#include <cmath>

#define CHANNELS 2
#define NUM_COMBS 8
#define NUM_APS 4

struct CombFilter {
    float *buffer;
    unsigned short buflen;
    int ind;
    float dampFactor;
    float decayFactor;
    float prev;
};

struct AllPassFilter {
    float *buffer;
    unsigned short buflen;
    int ind;
    float decayFactor;
};

struct SchroederReverb {
    struct CombFilter *Combs[CHANNELS][NUM_COMBS];
    struct AllPassFilter *APs[CHANNELS][NUM_APS];
    float wet;
    float wet1;
    float wet2;
    float width;
    float size;
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    float x1[CHANNELS];
    float x2[CHANNELS];
    float y1[CHANNELS];
    float y2[CHANNELS];
};

float applyComb(float input, struct CombFilter *Comb, float size);

float applyAP(float input, struct AllPassFilter *AP);

void applyReverb(struct SchroederReverb *R, float *left, float *right, float *wetL, float *wetR, int *wp, int numSamples, int wetBufSize, int nChannel);

float applyHPF(struct SchroederReverb *R, int ch, float x);

void freeReverb(struct SchroederReverb *R);

struct SchroederReverb *initReverb(float width, float wet, float dampFactor, float size);
