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

    class CustomTextEditor : public juce::TextEditor
    {
    public:
        class CustomMenuLookAndFeel : public juce::LookAndFeel_V4
        {
        public:
            CustomMenuLookAndFeel()
            {
                // Match the dark background color from the dialog
                juce::Colour bgColour(0x28, 0x28, 0x28);
                setColour(juce::PopupMenu::backgroundColourId, bgColour);
                setColour(juce::PopupMenu::textColourId, juce::Colours::white);
                setColour(juce::PopupMenu::highlightedBackgroundColourId, bgColour.brighter(0.2f));
                setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
            }

            juce::Font getPopupMenuFont() override
            {
                // Match the License Key label font (Arial, 16pt)
                return juce::Font("Arial", 16.0f, juce::Font::plain);
            }
        };

        CustomTextEditor()
        {
            customLookAndFeel = std::make_unique<CustomMenuLookAndFeel>();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu())
            {
                juce::PopupMenu menu;
                menu.setLookAndFeel(customLookAndFeel.get());

                bool hasSelection = getHighlightedRegion().getLength() > 0;
                bool hasText = getTotalNumChars() > 0;
                bool clipboardHasText = juce::SystemClipboard::getTextFromClipboard().isNotEmpty();

                menu.addItem(1, "Cut", hasSelection);
                menu.addItem(2, "Copy", hasSelection);
                menu.addItem(3, "Paste", clipboardHasText);
                menu.addSeparator();
                menu.addItem(4, "Select All", hasText);

                auto options = juce::PopupMenu::Options();
                menu.showMenuAsync(options,
                    [this](int result)
                    {
                        switch (result)
                        {
                            case 1: cutToClipboard(); break;
                            case 2: copyToClipboard(); break;
                            case 3: pasteFromClipboard(); break;
                            case 4: selectAll(); break;
                            default: break;
                        }
                    });
            }
            else
            {
                juce::TextEditor::mouseDown(e);
            }
        }

    private:
        std::unique_ptr<CustomMenuLookAndFeel> customLookAndFeel;
    };

    EqAudioProcessor& audioProcessor;
    LicenseSpring::LicenseManager::ptr_t licenseManager;
    juce::Label labelKeyBased{{}, "For key-based product enter"};
    juce::Label labelKey{{}, "License Key"};
    juce::TextButton activateKeyButton{"Activate"};
    juce::TextButton getTrialButton{"Get trial"};
    CustomTextEditor keyEditor;

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
