#pragma once

#include <stddef.h>
#include <juce_audio_processors/juce_audio_processors.h>

// LicenseSpring credentials (using macros for compile-time string encryption compatibility)
#define LS_API_KEY "fdf09a08-7b14-425d-8efd-5c4eb6237ba2"
#define LS_SHARED_KEY "5OSqYvr0blHXhSAwY9q0VWdsv7EYnxj76eA0zAgznAk"
#define LS_PRODUCT_CODE "Q-Invader-1"

namespace Constants {
    static constexpr int BUFFERSIZE = 32768;
    static constexpr int NUM_IRS = 18;
    static const juce::StringArray factoryPresets = { "The Rocker", "Capt. Crunch", "Eh-I-See!", "Soaring Lead", "Cleaning Up", "Sweet Tea Blues", "Rokk", "Crisp & Clear", "Modern Singles", "Modern Rhythms", "Anger Management", "Psychedlica", "Flying Solo", "Texas Blooze", "Vibin'", "Dreamscape", "Thrasher", "Down Under" };
    static int NUM_FACTORY_PRESETS = factoryPresets.size();


    // Display strings
    static constexpr const char* eq1display = "NEUTRALIZE\n";
    static constexpr const char* eq2display = "VAPORIZE\n";

    // UI Colors
    static const juce::Colour lightColour = juce::Colour(0x56, 0xf9, 0x7c);
}
