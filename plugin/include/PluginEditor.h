/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
//#include "Gui/PresetPanel.h"
#include "Gui/ButtonsAndKnobs.h"
#include "licenseChecker.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

//==============================================================================
/**
*/

//class ResizeButton : public juce::Button, juce::Button::Listener
//{
//public:
//    ResizeButton (const void* img, int size) {
//        buttonImage = juce::ImageFileFormat::loadFrom(img, size);
//    }
//    
//private:
//    juce::Image buttonImage;
//}


class EqAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::Button::Listener
{
public:
    EqAudioProcessorEditor (EqAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~EqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void setAmp(double gainLvl);
    bool writeModelDataIfNeeded(const juce::String& modelName, const void* data, int dataSize, unsigned long i);
    void updateToggleState (juce::Button* button);

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
    typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
    
    void switchAttachmentTo() {cc.switchAttachmentTo();};
    void setMainKnobVal(double val) {cc.setMainKnobVal(val);};
    void setButtonState(int lastBottomButton, int lastPresetButton) {
        cc.setButtonState(lastBottomButton, lastPresetButton);
    }
    void buttonClicked(juce::Button *button) override;
private:
    void showLicensePage();
    void hideLicensePage();
    class OverlayComponent : public juce::Component
    {
    public:
        OverlayComponent(std::function<void()> onOutsideClick, EqAudioProcessor& p) : onOutsideClickCallback(onOutsideClick), audioProcessor(p)
        {
            setInterceptsMouseClicks(true, true); // Block interactions with the background
        }

        void paint(juce::Graphics& g) override
        {
            // Draw a semi-transparent black overlay
            g.fillAll(juce::Colours::black.withAlpha(0.85f));
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            // Notify the parent component if the overlay is clicked
            if (onOutsideClickCallback) {
                onOutsideClickCallback();
            }
            audioProcessor.licenseVisibility.store(false);
        }

    private:
        std::function<void()> onOutsideClickCallback; // Callback to notify parent
        EqAudioProcessor& audioProcessor;
    };
    OverlayComponent overlay;
    juce::TextButton licenseButton{"License"};
    class NoOutlineLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
        {
            return;
        }

        void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool isButtonDown) override
        {
            return;
        }
    };
    double sizePortion = 1.0;
    void resizeButtonClicked() {
        return;
    }
//    std::unique_ptr<StandalonePluginHolder> pluginHolder;
    bool isStandalone = juce::JUCEApplicationBase::isStandaloneApp();
    AudioDeviceManager otherDeviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSettings;
    bool settingsVisibility = true;
    CustomLookAndFeel1 customLookAndFeel1;
    
    juce::Slider thicknessSlider;
    juce::Label thicknessLabel;
    std::unique_ptr<SliderAttachment> thicknessAttachment;
    
    juce::Slider presenceSlider;
    juce::Label presenceLabel;
    std::unique_ptr<SliderAttachment> presenceAttachment;
    
    juce::Slider ampRotary;
    std::unique_ptr<SliderAttachment> ampGainAttachment;
    
    juce::Slider reverbRotary;
    juce::Label reverbLabel;
    std::unique_ptr<SliderAttachment> reverbAttachment;
    
    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    std::unique_ptr<SliderAttachment> inputGainAttachment;
    
    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    
    juce::ToggleButton amp1Button {"switch to boost"};
    std::unique_ptr<ButtonAttachment> amp1Attachment;
    
    std::unique_ptr<juce::FileChooser> irChooser;
    std::unique_ptr<ComboBoxAttachment> irAttachment;
    std::unique_ptr<ComboBoxAttachment> userIRAttachment;
    juce::Label filePathLabel;
    bool isAudioSettingsVisible = false;
    int fullWidth = 980;
    int fullHeight = 980;
    bool isAmp1;
    bool ampOn;
    std::vector<std::shared_ptr<nam::DSP>> models;
    
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::ValueTree state;
    void setProcessorSizePortion(double size) {
        audioProcessor.sizePortion = size;
    }

    EqAudioProcessor& audioProcessor;
    
    juce::Label pitchLabel;
    juce::Label customIRLabel;

    juce::Slider standardASlider;
    juce::ToggleButton centHzButton {"Cents"};
    juce::ToggleButton muteButton {"Muted"};
    Gui::HorizontalMeter inMeter, outMeterL;
    Gui::clipIndicator inClip, outClipL;
    juce::Label inMeterLabel, outMeterLLabel;
    int cnt = 0;
    void timerCallback() override
    {
//        double currentPitch = audioProcessor.getCurrentPitch();
//        juce::String currentNote = audioProcessor.getCurrentNote();
//        double currentDev = audioProcessor.getCurrentDeviation();
//        // Format the pitch as a string and set it in the label
//        pitchLabel.setText (juce::String (currentPitch, 2) + " Hz (" + currentNote + " + " + juce::String (currentDev, 2) + ")", juce::dontSendNotification);
//        pitchLabel.setText (juce::String ("hello"), juce::dontSendNotification);
        float inMeterLevel = audioProcessor.getInRMS();
        float outMeterLeftLevel = audioProcessor.getOutRMS(0);
        inMeter.setLevel(inMeterLevel);
        inClip.setClip(inMeterLevel);
        outMeterL.setLevel(outMeterLeftLevel);
        outClipL.setClip(outMeterLeftLevel);
        inMeter.repaint();
        inClip.repaint();
        outMeterL.repaint();
        outClipL.repaint();

        // NOTE: Button state restoration now handled by CentralComponents::restoreButtonState()
        // Old restoration code using triggerClick() removed to prevent interference with boolean parameters
        // Removed lines that were reading and re-setting label text, which was preserving wrong label
    }
    bool ampOnState, amp1State, boostState, customIRState;

    std::unique_ptr<SliderAttachment> noiseGateSliderAttachment;
    juce::Slider noiseGateSlider;
    juce::Label noiseGateLabel;
    
    juce::Image background, mainDial, inGainDial, outGainDial, bgLight;

    CentralComponents cc;
    LicenseChecker licenseChecker;
    
    class ResizeButton : public juce::Button
    {
    public:
        ResizeButton(const void* img, int imgSize, const void* hoverImg, int hoverImgSize, double portion)
        : juce::Button("ResizeButton"), sizePortion(portion)
        {
            image = juce::ImageFileFormat::loadFrom(img, imgSize);
            hoverImage = juce::ImageFileFormat::loadFrom(hoverImg, hoverImgSize);
        }

        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            auto bounds = getLocalBounds().toFloat();
            // Use hover image if the mouse is over the button, otherwise use the normal image
            if (isMouseHovered)
                g.drawImage(hoverImage, bounds);
            else
                g.drawImage(image, bounds);
        }

        void mouseEnter(const juce::MouseEvent&) override
        {
            isMouseHovered = true;
            repaint();
        }

        void mouseExit(const juce::MouseEvent&) override
        {
            isMouseHovered = false;
            repaint();
        }
        double sizePortion;
    private:
        juce::Image image;
        juce::Image hoverImage;
        bool isMouseHovered = false; // Track whether the mouse is hovering over the button
    };
    
    ResizeButton resizeButton;
    
    class SettingsButton : public juce::Button
    {
    public:
        SettingsButton(const void* img, int imgSize, const void* hoverImg, int hoverImgSize, double portion)
        : juce::Button("SettingsButton"), sizePortion(portion)
        {
            image = juce::ImageFileFormat::loadFrom(img, imgSize);
            hoverImage = juce::ImageFileFormat::loadFrom(hoverImg, hoverImgSize);
        }

        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            auto bounds = getLocalBounds().toFloat();
            // Use hover image if the mouse is over the button, otherwise use the normal image
            if (isMouseHovered)
                g.drawImage(hoverImage, bounds);
            else
                g.drawImage(image, bounds);
        }

        void mouseEnter(const juce::MouseEvent&) override
        {
            isMouseHovered = true;
            repaint();
        }

        void mouseExit(const juce::MouseEvent&) override
        {
            isMouseHovered = false;
            repaint();
        }
        double sizePortion;
    private:
        juce::Image image;
        juce::Image hoverImage;
        bool isMouseHovered = false; // Track whether the mouse is hovering over the button
    };
    class ClickableLabel : public juce::Label
    {
    public:
        ClickableLabel(const juce::String& name, const juce::String& labelText)
            : juce::Label(name, labelText) {}

        // Setter for the click callback
        void setClickCallback(std::function<void()> onClickFunction)
        {
            onClick = std::move(onClickFunction);
        }

        void mouseUp(const juce::MouseEvent& event) override
        {
            if (onClick)
                onClick(); // Call the callback function when the label is clicked
        }

    private:
        std::function<void()> onClick;
    };
    void settingsButtonClicked()
    {
        // This function mimics buttonClicked's functionality for settingsButton
        audioSettings->setVisible(settingsVisibility);
        audioSettingsBackground.setVisible(settingsVisibility);
        settingsLabel.setVisible(settingsVisibility);
        closeSettingsLabel.setVisible(settingsVisibility);
        settingsVisibility = !settingsVisibility;
        resized();
    }
    SettingsButton settingsButton;
    juce::TextEditor audioSettingsBackground;
    juce::Label settingsLabel;
    ClickableLabel closeSettingsLabel;
    // Override the mouseDown method in the main component class
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (settingsVisibility) // Only check when settings are visible
        {
            // Get the combined bounds of the settings section
            auto settingsBounds = audioSettingsBackground.getBounds();
            
            // Check if the click is outside the settings section
            if (!settingsBounds.contains(event.getPosition()))
            {
                settingsButtonClicked(); // Hide settings if click is outside
            }
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqAudioProcessorEditor)
};
