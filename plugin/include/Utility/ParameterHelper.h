#pragma once

//#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "defines.h"

using namespace juce;

namespace Utility
{
	class ParameterHelper
	{
	public:
		ParameterHelper() = delete;

		static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
		{
			return AudioProcessorValueTreeState::ParameterLayout{
				std::make_unique<AudioParameterFloat>("input gain", "Input Gain", NormalisableRange<float>{-20.f, 20.f, 0.1f}, 0.f),
				std::make_unique<AudioParameterFloat>("output gain", "Output Gain", NormalisableRange<float>{-20.f, 20.f, 0.1f}, 0.f),
                std::make_unique<AudioParameterFloat>("eq2", "eq2", NormalisableRange<float>{-6.f, 6.f, 0.1f}, 0.f),
                std::make_unique<AudioParameterFloat>("eq1", "eq1", NormalisableRange<float>{-6.f, 6.f, 0.1f}, 0.f),
                std::make_unique<AudioParameterFloat>("reverb", "Reverb", NormalisableRange<float>{0.f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterFloat>("delay mix", "Delay Mix", NormalisableRange<float>{0.f, 1.f, 0.01f}, 0.f),
                std::make_unique<AudioParameterFloat>("delay feedback", "Delay Feedback", NormalisableRange<float>{0.0f, 1.f, 0.01f}, 0.3f),
                std::make_unique<AudioParameterInt>("delay timing", "Delay Timing", 0, 16, 10),
                std::make_unique<AudioParameterFloat>("amp gain", "Amp Gain", NormalisableRange<float>{1.f, 10.f, 0.1f}, 1.f),
                std::make_unique<AudioParameterFloat>("boost gain", "Boost Amp Gain", NormalisableRange<float>{1.f, 10.f, 0.5f}, 1.f),
                std::make_unique<AudioParameterBool>("is amp 1", "is amp 1", false),
                std::make_unique<AudioParameterBool>("is eq 1", "is eq 1", false),
                std::make_unique<AudioParameterBool>("is fx 1", "is fx 1", false),
                std::make_unique<AudioParameterBool>("amp smooth", "Amp Smooth", false),
                std::make_unique<AudioParameterInt>("ir selection", "IR Selection", 0, Constants::NUM_IRS, 0),
                std::make_unique<AudioParameterInt>("custom ir selection", "Custom IR Selection", 1, 100, 1),
                std::make_unique<AudioParameterInt>("model id", "Model Index", 0, 100, 0),
                std::make_unique<AudioParameterFloat>("noise gate", "Noise Gate", NormalisableRange<float>{-100.f, 0.f, 0.1f}, -80.f),
                
                std::make_unique<AudioParameterBool>("p1 state", "P1 State", true),
                std::make_unique<AudioParameterBool>("p2 state", "P2 State", false),
                std::make_unique<AudioParameterBool>("p3 state", "P3 State", false),
                std::make_unique<AudioParameterBool>("p4 state", "P4 State", false),
                std::make_unique<AudioParameterBool>("p5 state", "P5 State", false),
                std::make_unique<AudioParameterBool>("gain state", "Gain State", false),
                std::make_unique<AudioParameterBool>("gate state", "Gate State", false),
                std::make_unique<AudioParameterBool>("eq state", "EQ State", false),
                std::make_unique<AudioParameterBool>("ir visibility", "IR Visibility", false),
                std::make_unique<AudioParameterBool>("reverb state", "Reverb State", false),
                std::make_unique<AudioParameterBool>("delay state", "Delay State", false),
                std::make_unique<AudioParameterBool>("fx state", "FX State", false),
                std::make_unique<AudioParameterBool>("back state", "Back State", false)
			};
		}
	};
}
