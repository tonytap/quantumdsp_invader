#pragma once

#include <stddef.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace Constants {
    static constexpr int BUFFERSIZE = 32768;
    static constexpr int NUM_IRS = 18;
    static constexpr int NUM_FACTORY_PRESETS = 18;
    static constexpr StringArray factoryPresets = { "The Rocker", "Capt. Crunch", "Eh-I-See!", "Soaring Lead", "Cleaning Up", "Sweet Tea Blues", "Rokk", "Crisp & Clear", "Modern Singles", "Modern Rhythms", "Anger Management", "Psychedlica", "Flying Solo", "Texas Blooze", "Vibin'", "Dreamscape", "Thrasher", "Down Under" };
}
