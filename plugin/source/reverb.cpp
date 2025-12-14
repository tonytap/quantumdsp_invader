/*
  ==============================================================================

    reverb.cpp
    Created: 18 May 2024 12:33:00pm
    Author:  Anthony Hong

  ==============================================================================
*/

#include "reverb.h"
#include <stdlib.h>
#include <stdio.h>

float applyComb(float input, struct CombFilter *Comb, float size) {
    float output = Comb->buffer[Comb->ind];
    Comb->decayFactor = 0.7f+0.28f*size;
    Comb->prev = Comb->dampFactor*Comb->prev+output*(1.0f-Comb->dampFactor);
    Comb->buffer[Comb->ind] = Comb->decayFactor*Comb->prev+input;
    Comb->ind = (Comb->ind+1)%Comb->buflen;
    return output;
}

float applyAP(float input, struct AllPassFilter *AP) {
    float bufferValue = AP->buffer[AP->ind];
    float output = bufferValue;
    AP->buffer[AP->ind] = input;
    AP->ind = (AP->ind+1)%AP->buflen;
    return output;
}

void applyReverb(struct SchroederReverb *R, float *left, float *right, float *wetL, float *wetR, int *wp, int numSamples, int wetBufSize, int nChannel) {
    float dryMix = 1.0-R->wet/3.0;
    for (int i = 0; i < numSamples; ++i)
    {
        float input = (left[i] + right[i]) * 0.015f;
        float outL = 0, outR = 0;
        
        // parallel comb filters
        for (int j = 0; j < NUM_COMBS; ++j)
        {
            outL += applyComb(input, R->Combs[0][j], R->size);
            outR += applyComb(input, R->Combs[1][j], R->size);
        }

        // serial all-pass filters
        for (int j = 0; j < NUM_APS; ++j)
        {
            outL = applyAP(outL, R->APs[0][j]);
            outR = applyAP(outR, R->APs[1][j]);
        }
        R->wet1 = 0.5f*R->wet*(1.0f+R->width);
        R->wet2 = 0.5f*R->wet*(1.0f-R->width);
        left[i] *= dryMix;
        right[i] *= dryMix;
        int val = *wp;
        wetL[val] = applyHPF(R, 0, R->wet*(outL * R->wet1 + outR * R->wet2));
        wetR[val] = applyHPF(R, 1, R->wet*(outR * R->wet1 + outL * R->wet2));
        if (nChannel == 1) {
            wetL[val] = 0.5*wetL[val]+0.5*wetR[val];
        }
        val++;
        if (val >= wetBufSize) {
            val = 0;
        }
        *wp = val;
    }
}

float applyHPF(struct SchroederReverb *R, int ch, float x) {
    float y = R->b0*x+R->b1*R->x1[ch]+R->b2*R->x2[ch]-R->a1*R->y1[ch]-R->a2*R->y2[ch];
    R->x2[ch] = R->x1[ch];
    R->x1[ch] = x;
    R->y2[ch] = R->y1[ch];
    R->y1[ch] = y;
    return y;
}

void freeReverb(struct SchroederReverb *R) {
    for (int i = 0; i < CHANNELS; i++) {
        for (int j = 0; j < NUM_COMBS; j++) {
            free(R->Combs[i][j]->buffer);
        }
        free(R->Combs);
        for (int j = 0; j < NUM_APS; j++) {
            free(R->APs[i][j]->buffer);
        }
        free(R->APs);
    }
    free(R);
}

struct SchroederReverb *initReverb(float width, float wet, float dampFactor, float size) {
    struct SchroederReverb *R = (struct SchroederReverb *)malloc(sizeof(struct SchroederReverb));
    if (R == NULL) {
        printf("Schroeder reverb allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    R->wet = wet*3.0f;
    R->width = width;
    R->wet1 = 0.5f*R->wet*(1.0f+R->width);
    R->wet2 = 0.5f*R->wet*(1.0f-R->width);
    R->size = size;
    unsigned short CombsDelays[NUM_COMBS] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    unsigned short APDelays[NUM_APS] = {556, 441, 341, 225};
    for (int i = 0; i < CHANNELS; i++) {
        for (int j = 0; j < NUM_COMBS; j++) {
            R->Combs[i][j] = (struct CombFilter*)malloc(sizeof(struct CombFilter));
            if (R->Combs[i][j] == NULL) {
                printf("Comb filter allocation failed!\n");
                exit(EXIT_FAILURE);
            }
            R->Combs[i][j]->dampFactor = dampFactor*0.4f;
            R->Combs[i][j]->decayFactor = 0.7f+0.28f*R->size;
            if (i == 1) {
                R->Combs[i][j]->buflen = 23+CombsDelays[j];
            } else {
                R->Combs[i][j]->buflen = CombsDelays[j];
            }
            R->Combs[i][j]->ind = 0;
            R->Combs[i][j]->prev = 0.0f;
            R->Combs[i][j]->buffer = (float*)calloc(sizeof(float), R->Combs[i][j]->buflen);
            if (R->Combs[i][j]->buffer == NULL) {
                printf("Comb filter buffer allocation failed!\n");
                exit(EXIT_FAILURE);
            }
            for (unsigned short s = 0; s < R->Combs[i][j]->buflen; s++) {
                R->Combs[i][j]->buffer[s] = 0;
            }
        }
        for (int j = 0; j < NUM_APS; j++) {
            R->APs[i][j] = (struct AllPassFilter*)malloc(sizeof(struct AllPassFilter));
            if (R->APs[i][j] == NULL) {
                printf("All-pass filter allocation failed!\n");
                exit(EXIT_FAILURE);
            }
            if (i == 1) {
                R->APs[i][j]->buflen = 23+APDelays[j];
            } else {
                R->APs[i][j]->buflen = APDelays[j];
            }
            R->APs[i][j]->ind = 0;
            R->APs[i][j]->buffer = (float*)calloc(sizeof(float), R->APs[i][j]->buflen);
            if (R->APs[i][j]->buffer == NULL) {
                printf("All-pass filter buffer allocation failed!\n");
                exit(EXIT_FAILURE);
            }
            for (unsigned short s = 0; s < R->APs[i][j]->buflen; s++) {
                R->APs[i][j]->buffer[s] = 0;
            }
        }
    }
    float fs = 44100.0;
    float fc = 100.0;
    float K = tan(M_PI*fc/fs);
    float Q = 1/sqrt(2);
    R->b0 = Q/(K*K*Q+K+Q);
    R->b1 = -2*Q/(K*K*Q+K+Q);
    R->b2 = Q/(K*K*Q+K+Q);
    R->a1 = 2*Q*(K*K-1)/(K*K*Q+K+Q);
    R->a2 = (K*K*Q-K+Q)/(K*K*Q+K+Q);
    for (int ch = 0; ch < CHANNELS; ch++) {
        R->x1[ch] = 0;
        R->x2[ch] = 0;
        R->y1[ch] = 0;
        R->y2[ch] = 0;
    }
    return R;
}
