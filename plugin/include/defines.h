#pragma once

#include <stddef.h>
#include <juce_audio_processors/juce_audio_processors.h>

// LicenseSpring credentials (using macros for compile-time string encryption compatibility)
#define LS_API_KEY "fdf09a08-7b14-425d-8efd-5c4eb6237ba2"
#define LS_SHARED_KEY "5OSqYvr0blHXhSAwY9q0VWdsv7EYnxj76eA0zAgznAk"
#define LS_PRODUCT_CODE "Q-Invader-1"

namespace Constants {
    static constexpr int BUFFERSIZE = 8192;
    static constexpr int NUM_IRS = 18;
    static const juce::StringArray factoryPresets = { "The Rocker", "Capt. Crunch", "Eh-I-See!", "Soaring Lead", "Cleaning Up", "Sweet Tea Blues", "Rokk", "Crisp & Clear", "Modern Singles", "Modern Rhythms", "Anger Management", "Psychedlica", "Flying Solo", "Texas Blooze", "Vibin'", "Dreamscape", "Thrasher", "Down Under", "Golden Overdrive", "Neon Drive", "Sugar & Fire", "Static Motion", "Echo Canyon", "Sunset Rebel" };
    static int NUM_FACTORY_PRESETS = factoryPresets.size();
 
    // Display strings
    static constexpr const char* eq1display = "NEUTRALIZE\n";
    static constexpr const char* eq2display = "VAPORIZE\n";

    static constexpr const char* productName = "Invader";
    static constexpr const char* versionNum = "1.2.0";

    // UI Colors
    static const juce::Colour lightColour = juce::Colour(0x56, 0xf9, 0x7c);

    static constexpr float eq1_1_slope = 1.f;
    static constexpr float eq1_1_bias = 0.f;
    static constexpr float eq1_2_slope = 2.f;
    static constexpr float eq1_2_bias = 0.f;
    static constexpr float eq2_1_slope = 2.f;
    static constexpr float eq2_1_bias = 0.f;
    static constexpr float eq2_2_slope = 1.f;
    static constexpr float eq2_2_bias = 2.f;
    static constexpr float fc_globalEQ = 5200;
    static constexpr float gain_globalEQ = 1.6f;
    static constexpr float fc_eq1_1 = 120;
    static constexpr float fc_eq1_2 = 800;
    static constexpr float fc_eq2_1 = 2000;
    static constexpr float fc_eq2_2 = 5000;
}
