#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

using namespace juce;

namespace Service
{
	class PresetManager : ValueTree::Listener
	{
	public:
		static const File defaultDirectory;
		static const String extension;
		static const String presetNameProperty;

		PresetManager(AudioProcessorValueTreeState&);

		void savePreset(const String& presetName);
        void saveFactoryPreset(const String& presetName, std::unique_ptr<juce::XmlElement> xmlElement) {
            if (presetName.isEmpty())
                return;

            currentPreset.setValue(presetName);
            const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
            if (!xmlElement->writeTo(presetFile))
            {
                DBG("Could not create preset file: " + presetFile.getFullPathName());
                jassertfalse;
            }
        }
		void deletePreset(const String& presetName);
		void loadPreset(const String& presetName);
		int loadNextPreset();
		int loadPreviousPreset();
		StringArray getAllPresets() const;
		String getCurrentPreset() const;
        std::tuple<StringArray, StringArray> getFactoryPresets(StringArray presets) const;
	private:
		void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override;
		AudioProcessorValueTreeState& valueTreeState;
		Value currentPreset;
	};
}
