/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

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
#include <LicenseSpring/LicenseManager.h>
#include "PluginProcessor.h"

class LicenseChecker  : public juce::Component, public juce::TextEditor::Listener, public juce::Button::Listener
{
public:
    //==============================================================================
    LicenseChecker( LicenseSpring::LicenseManager::ptr_t manager, EqAudioProcessor& p);
    ~LicenseChecker() override;
    void buttonClicked( juce::Button* ) override;
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    double sizePortion = 1.0;
    void makeExpirationInfoVisible();
    void updateLicenseFields();
    void updateLicenseFieldsWithResize(float sizeFactor);
private:
    //==============================================================================
    class StyledLabel : public juce::Label
    {
    public:
        // Method to set the attributed string
        void setAttributedText(const juce::AttributedString& newText)
        {
            attributedText = newText;
            repaint(); // Trigger a repaint to render the new text
        }

        // Override paint method to draw the attributed string
        void paint(juce::Graphics& g) override
        {
            if (!attributedText.getText().isEmpty())
            {
                attributedText.draw(g, getLocalBounds().toFloat());
            }
            else
            {
                juce::Label::paint(g); // Fallback to default Label painting
            }
        }

    private:
        juce::AttributedString attributedText; // Store the attributed string
    };
    EqAudioProcessor& audioProcessor;
    LicenseSpring::LicenseManager::ptr_t licenseManager;
    juce::Label labelKeyBased{{}, "For key-based product enter"};
    juce::Label labelKey{{}, "License Key"};
    juce::TextButton activateKeyButton{"Activate"};
    juce::TextButton getTrialButton{"Get trial"};
    juce::TextEditor keyEditor;

    StyledLabel labelInfo;
    juce::TextButton deactivateButton{"Deactivate"};
    juce::TextButton backButton{"Back"};
    juce::TextButton proceedButton{"Proceed"};

    void activateKeyBased();
    void activateUserBased();
    void getTrial();
    void deactivate();
    void checkLicense();
    void makeInfoVisible();
    void makeActivationVisible();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseChecker)
};
