#pragma once

//#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Gui/ButtonsAndKnobs.h"

inline float dropdownFontSize = 15.f;

namespace Gui
{
    class CustomLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        CustomLookAndFeel()
        {
            juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
            setColour(juce::ComboBox::backgroundColourId, bgColour);
            setColour(juce::PopupMenu::backgroundColourId, bgColour);
            setColour(juce::TextButton::buttonColourId, bgColour);
        }
        // Override to set the combo box item text font
        juce::Font getComboBoxFont(juce::ComboBox& comboBox) override
        {
            comboBoxHeight = comboBox.getHeight();
            return juce::Font("Arial", comboBoxHeight*0.5f, juce::Font::plain);
        }
        // Override to set the font for all items in the ComboBox dropdown
        juce::Font getPopupMenuFont() override
        {
            return juce::Font("Arial", comboBoxHeight*0.5f, juce::Font::plain); // Set font to Arial, size 14 for dropdown items
        }
        // Override to set the font for button text
        juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
        {
            return juce::Font("Arial", buttonHeight*0.5f, juce::Font::plain); // Set font to Arial, size 14
        }
        float comboBoxHeight;
    };
    class HorizontalMeter : public Component
    {
    public:

        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();

            g.setColour(Colours::black);
            g.fillRect(bounds);

            g.setColour(Colours::white);
            const auto scaledY = jmap(level, -60.f, 6.f, 0.f, static_cast<float>(getHeight()));
            g.fillRect(bounds.removeFromBottom(scaledY));
        }
        
        void setLevel(const float value) { level = value; }
    private:
        float level = -60.f;
    };

    class clipIndicator : public HorizontalMeter
    {
    public:
        void paint(Graphics& g) override
        {
            auto bounds = getLocalBounds();
            g.setColour(juce::Colours::black);
            g.setColour(stayClipped ? juce::Colours::red : juce::Colours::black);
            g.fillRect(bounds.removeFromTop(20)); // Example size for the clipping indicator
        }
        void mouseDown(const juce::MouseEvent& event) override
        {
            if (stayClipped)
            {
                stayClipped = false;
                repaint();
            }
        }
        void setClip(const float value) {
            clipping = value >= 0.f;
            if (clipping) {
                stayClipped = true;
                juce::Timer::callAfterDelay (5000, [this]()
                {
                    stayClipped = false;
                    repaint();
                });
            }
        }
    private:
        bool clipping = false; // Local flag for the GUI
        bool stayClipped = false;
    };

	class PresetPanel : public Component, Button::Listener, ComboBox::Listener
	{
	public:
		PresetPanel(Service::PresetManager& pm, EqAudioProcessor& p, juce::Label *lbl = nullptr, juce::Label *valLbl = nullptr) : presetManager(pm), audioProcessor(p)
		{
			configureButton(saveButton, "Save");
			configureButton(deleteButton, "Delete");
			configureButton(previousPresetButton, "<");
			configureButton(nextPresetButton, ">");
            
			presetList.setTextWhenNothingSelected("Factory presets");
			presetList.setMouseCursor(MouseCursor::PointingHandCursor);
			addAndMakeVisible(presetList);
			presetList.addListener(this);
            
            userPresetList.setTextWhenNothingSelected("Custom presets");
            userPresetList.setMouseCursor(MouseCursor::PointingHandCursor);
            addAndMakeVisible(userPresetList);
            userPresetList.addListener(this);
			loadPresetList();
            presetList.setLookAndFeel(&customLookAndFeel);
            userPresetList.setLookAndFeel(&customLookAndFeel);
            saveButton.setLookAndFeel(&customLookAndFeel);
            deleteButton.setLookAndFeel(&customLookAndFeel);
            previousPresetButton.setLookAndFeel(&customLookAndFeel);
            nextPresetButton.setLookAndFeel(&customLookAndFeel);
            label = lbl;
            valueLabel = valLbl;
            juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
            presetBackground.setColour(juce::TextEditor::backgroundColourId, bgColour);  // Set background color
            addAndMakeVisible(presetBackground);
		}

		~PresetPanel()
		{
			saveButton.removeListener(this);
			deleteButton.removeListener(this);
			previousPresetButton.removeListener(this);
			nextPresetButton.removeListener(this);
			presetList.removeListener(this);
            userPresetList.removeListener(this);
		}

		void resized() override
		{
            auto container = getLocalBounds().reduced(4);
            auto bounds = container;
            saveButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.2f)).reduced(4));
            previousPresetButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.1f)).reduced(4));
            presetList.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.2f)).reduced(4));
            userPresetList.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.2f)).reduced(4));
            nextPresetButton.setBounds(bounds.removeFromLeft(container.proportionOfWidth(0.1f)).reduced(4));
            deleteButton.setBounds(bounds.reduced(4));
		}
        
        void setPresetNum(juce::String& name, bool isEdited) {
            for (int i = 1; i <= presetList.getNumItems(); i++) {
                juce::String itemText = presetList.getItemText(i-1);
                DBG("item: " << itemText << ", target: " << name);
                if (itemText == name && isEdited) {
                    presetList.setSelectedId(i, juce::dontSendNotification);
                    userPresetList.setSelectedId(-1, juce::dontSendNotification);
                    return;
                }
                else if (itemText == name && !isEdited) {
                    presetList.setSelectedId(i);
                    return;
                }
            }
            for (int i = 1; i <= userPresetList.getNumItems(); i++) {
                juce::String itemText = userPresetList.getItemText(i-1);
                DBG("item: " << itemText << ", target: " << name);
                if (itemText == name && isEdited) {
                    userPresetList.setSelectedId(i, juce::dontSendNotification);
                    presetList.setSelectedId(-1, juce::dontSendNotification);
                    return;
                }
                else if (itemText == name && !isEdited) {
                    userPresetList.setSelectedId(i);
                    return;
                }
            }
        }
        
        ComboBox& getFactoryDropdown() {
            return presetList;
        }
        
        ComboBox& getUserDropdown() {
            return userPresetList;
        }
        int buttonSelected = 1;
        CustomLookAndFeel customLookAndFeel;
        juce::Label *label = nullptr;
        juce::Label *valueLabel = nullptr;
        TextButton saveButton, deleteButton, previousPresetButton, nextPresetButton;
        juce::TextEditor presetBackground;
        ComboBox presetList;
        ComboBox userPresetList;
	private:
        void extraPresetConfig() {
            // This function restores GUI state after loadPreset() has updated valueTreeState
            // It should work identically whether called from:
            // 1. User selecting preset via combo box
            // 2. First launch loading default preset
            // 3. Preset button click
            //
            // loadPreset() has already loaded ALL state into valueTreeState,
            // so we just need to sync the GUI with that state.

            auto state = audioProcessor.valueTreeState.state;

            // === IR Restoration ===
            // Use unified helper method for IR restoration
            audioProcessor.restoreIRFromState();

            // === Button State Restoration ===
            // Read from loaded state, don't hardcode
            audioProcessor.lastBottomButton = 0;
//            audioProcessor.lastPresetButton = state.getProperty("lastPresetButton", 0);
            audioProcessor.restoreEditorButtonState();
        }
        
		void buttonClicked(Button* button) override
		{
			if (button == &saveButton)
			{
				fileChooser = std::make_unique<FileChooser>(
					"Please enter the name of the preset to save",
					Service::PresetManager::defaultDirectory,
					"*." + Service::PresetManager::extension
				);
				fileChooser->launchAsync(FileBrowserComponent::saveMode, [&](const FileChooser& chooser)
					{
                        audioProcessor.valueTreeState.state.setProperty("irOption", audioProcessor.irDropdown.getSelectedId(), nullptr);
                        audioProcessor.valueTreeState.state.setProperty("customIROption", audioProcessor.userIRDropdown.getSelectedId(), nullptr);
                        audioProcessor.valueTreeState.state.setProperty("currentMainKnobID", audioProcessor.currentMainKnobID, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("savedButton", audioProcessor.savedButtonID, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("irVisibility", audioProcessor.irVisibility, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("p1n", audioProcessor.p1n, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("p2n", audioProcessor.p2n, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("p3n", audioProcessor.p3n, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("p4n", audioProcessor.p4n, nullptr);
                        audioProcessor.valueTreeState.state.setProperty("p5n", audioProcessor.p5n, nullptr);
						const auto resultFile = chooser.getResult();
						presetManager.savePreset(resultFile.getFileNameWithoutExtension());
						loadPresetList(false, juce::sendNotificationAsync);
					});
			}
			if (button == &previousPresetButton)
			{
				const auto index = presetManager.loadPreviousPreset();
                setPresetItemIndex(index);
			}
			if (button == &nextPresetButton)
			{
				const auto index = presetManager.loadNextPreset();
                setPresetItemIndex(index);
			}
			if (button == &deleteButton)
			{
				presetManager.deletePreset(presetManager.getCurrentPreset());
				loadPresetList(true);
			}
		}
        
		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
		{
            audioProcessor.presetSmoothing.store(true);
            juce::String presetName;
			if (comboBoxThatHasChanged == &presetList)
			{
                userPresetList.setSelectedItemIndex(-1, juce::dontSendNotification);
                juce::String name = presetList.getItemText(presetList.getSelectedItemIndex());
                presetName = name;
				presetManager.loadPreset(name);
                if (buttonSelected == 1) {
                    audioProcessor.p1n = name;
                }
                else if (buttonSelected == 2) {
                    audioProcessor.p2n = name;
                }
                else if (buttonSelected == 3) {
                    audioProcessor.p3n = name;
                }
                else if (buttonSelected == 4) {
                    audioProcessor.p4n = name;
                }
                else if (buttonSelected == 5) {
                    audioProcessor.p5n = name;
                }
                label->setText(presetName, juce::dontSendNotification);
			}
            else if (comboBoxThatHasChanged == &userPresetList) {
                presetList.setSelectedItemIndex(-1, juce::dontSendNotification);
                juce::String name = userPresetList.getItemText(userPresetList.getSelectedItemIndex());
                presetName = name;
                presetManager.loadPreset(name);
                if (buttonSelected == 1) {
                    audioProcessor.p1n = name;
                }
                else if (buttonSelected == 2) {
                    audioProcessor.p2n = name;
                }
                else if (buttonSelected == 3) {
                    audioProcessor.p3n = name;
                }
                else if (buttonSelected == 4) {
                    audioProcessor.p4n = name;
                }
                else if (buttonSelected == 5) {
                    audioProcessor.p5n = name;
                }
            }
            // After loadPreset(), extraPresetConfig() handles IR loading and button init
            extraPresetConfig();
            valueLabel->setText("", juce::dontSendNotification);
            label->setText(presetName, juce::dontSendNotification);
		}

		void configureButton(Button& button, const String& buttonText) 
		{
			button.setButtonText(buttonText);
			button.setMouseCursor(MouseCursor::PointingHandCursor);
			addAndMakeVisible(button);
			button.addListener(this);
		}
        
        void setPresetItemIndex(int preset_id, juce::NotificationType notification = juce::sendNotificationAsync) {
            if (preset_id < Constants::NUM_FACTORY_PRESETS) {
                presetList.setSelectedItemIndex(preset_id, notification);
                userPresetList.setSelectedItemIndex(-1, juce::dontSendNotification);
            }
            else {
                userPresetList.setSelectedItemIndex(preset_id-Constants::NUM_FACTORY_PRESETS, notification);
                presetList.setSelectedItemIndex(-1, juce::dontSendNotification);
            }
        }

		void loadPresetList(bool deselectAllPresets = false, juce::NotificationType notification = juce::dontSendNotification)
		{
			presetList.clear(dontSendNotification);
			const auto allPresets = presetManager.getAllPresets();
			const auto currentPreset = presetManager.getCurrentPreset();
            const auto [factoryPresets, userPresets] = presetManager.getFactoryPresets(allPresets);
			presetList.addItemList(factoryPresets, 1);
            userPresetList.clear(dontSendNotification);
            userPresetList.addItemList(userPresets, 1);
            int preset_id = allPresets.indexOf(currentPreset);
            if (!deselectAllPresets) {
                setPresetItemIndex(preset_id, notification);
            }
            else {
                setPresetItemIndex(-1, juce::dontSendNotification);
            }
		}

		Service::PresetManager& presetManager;
		std::unique_ptr<FileChooser> fileChooser;
        EqAudioProcessor& audioProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
	};
}
