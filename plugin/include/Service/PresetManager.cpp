#include "PresetManager.h"
#include "../PluginProcessor.h"

namespace Service
{
	const File PresetManager::defaultDirectory{ File::getSpecialLocation(
		File::SpecialLocationType::commonDocumentsDirectory)
			.getChildFile("Quantum DSP")
			.getChildFile("Invader Presets")
	};
	const String PresetManager::extension{ "preset" };
	const String PresetManager::presetNameProperty{ "presetName" };

	PresetManager::PresetManager(AudioProcessorValueTreeState& apvts) :
		valueTreeState(apvts)
	{
		// Create a default Preset Directory, if it doesn't exist
		if (!defaultDirectory.exists())
		{
			const auto result = defaultDirectory.createDirectory();
			if (result.failed())
			{
				DBG("Could not create preset directory: " + result.getErrorMessage());
				jassertfalse;
			}
		}

		valueTreeState.state.addListener(this);
		currentPreset.referTo(valueTreeState.state.getPropertyAsValue(presetNameProperty, nullptr));
	}

	void PresetManager::savePreset(const String& presetName)
	{
		if (presetName.isEmpty())
			return;

		currentPreset.setValue(presetName);
		const auto xml = valueTreeState.copyState().createXml();
		const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
//        DBG(xml->toString());
//        DBG(valueTreeState.state.toXmlString());
		if (!xml->writeTo(presetFile))
		{
			DBG("Could not create preset file: " + presetFile.getFullPathName());
			jassertfalse;
		}
	}

	void PresetManager::deletePreset(const String& presetName)
	{
		if (presetName.isEmpty())
			return;

		const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
		if (!presetFile.existsAsFile())
		{
			DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
			jassertfalse;
			return;
		}
		if (!presetFile.deleteFile())
		{
			DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
			jassertfalse;
			return;
		}
		currentPreset.setValue("");
	}

	void PresetManager::loadPreset(const String& presetName)
	{
		if (presetName.isEmpty())
			return;

		const auto presetFile = defaultDirectory.getChildFile(presetName + "." + extension);
		if (!presetFile.existsAsFile())
		{
			DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
			jassertfalse;
			return;
		}
		// presetFile (XML) -> (ValueTree)
		XmlDocument xmlDocument{ presetFile };
        auto docElem = *xmlDocument.getDocumentElement();
		const auto valueTreeToLoad = ValueTree::fromXml(docElem);

		valueTreeState.replaceState(valueTreeToLoad);
		currentPreset.setValue(presetName);
	}

	int PresetManager::loadNextPreset()
	{
        const auto allPresets = getAllPresets();
//        const auto [factoryPresets, userPresets] = getFactoryPresets(allPresets);
//        StringArray allPresets = factoryPresets;
//        allPresets.addArray(userPresets);
        if (allPresets.isEmpty())
            return -1;
		const auto currentIndex = allPresets.indexOf(currentPreset.toString());
		const auto nextIndex = currentIndex + 1 > (allPresets.size() - 1) ? 0 : currentIndex + 1;
		loadPreset(allPresets.getReference(nextIndex));
		return nextIndex;
	}

	int PresetManager::loadPreviousPreset()
	{
        const auto allPresets = getAllPresets();
        if (allPresets.isEmpty())
            return -1;
		const auto currentIndex = allPresets.indexOf(currentPreset.toString());
		const auto previousIndex = currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
		loadPreset(allPresets.getReference(previousIndex));
		return previousIndex;
	}

	StringArray PresetManager::getAllPresets() const
	{
        StringArray presets;
        presets.addArray(factoryPresets);
        const auto fileArray = defaultDirectory.findChildFiles(
            File::TypesOfFileToFind::findFiles, false, "*." + extension);

        Array<juce::File> userPresetFiles;
        for (const auto& file : fileArray)
        {
            String presetName = file.getFileNameWithoutExtension();
            if (!factoryPresets.contains(presetName))
            {
                userPresetFiles.add(file);
            }
        }
        std::sort(userPresetFiles.begin(), userPresetFiles.end(), [](const juce::File& a, const juce::File& b) {
            return a.getCreationTime() > b.getCreationTime();
        });
        for (const auto& file : userPresetFiles)
        {
            presets.add(file.getFileNameWithoutExtension());
        }
        return presets;
	}

    std::tuple<StringArray, StringArray> PresetManager::getFactoryPresets(StringArray presets) const
    {
        StringArray factoryPresets;
        StringArray userPresets;

        // Get up to the first 5 elements for factory presets
        int numFactoryPresets = std::min(NUM_FACTORY_PRESETS, presets.size());

        for (int i = 0; i < numFactoryPresets; ++i)
        {
            factoryPresets.add(presets[i]);
        }

        // Remaining elements are considered user presets
        for (int i = numFactoryPresets; i < presets.size(); ++i)
        {
            userPresets.add(presets[i]);
        }

        // Return both the factory and user presets as a tuple
        return std::make_tuple(factoryPresets, userPresets);
    }

	String PresetManager::getCurrentPreset() const
	{
		return currentPreset.toString();
	}

	void PresetManager::valueTreeRedirected(ValueTree& treeWhichHasBeenChanged)
	{
		currentPreset.referTo(treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr));
	}
}
