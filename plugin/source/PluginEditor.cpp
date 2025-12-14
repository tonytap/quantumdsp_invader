/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
EqAudioProcessorEditor::EqAudioProcessorEditor (EqAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p),
      valueTreeState (vts),
      audioProcessor (p),
      overlay([this]() { hideLicensePage(); }, p),
      cc(BinaryData::buttonbigdialV4_png, BinaryData::buttonbigdialV4_pngSize, p, vts, BinaryData::rotarydial_V2_png, BinaryData::rotarydial_V2_pngSize),
      resizeButton(BinaryData::resize_png, BinaryData::resize_pngSize, BinaryData::resizehover_png, BinaryData::resizehover_pngSize, p.sizePortion),
      settingsButton(BinaryData::settingsgear_png, BinaryData::settingsgear_pngSize, BinaryData::settingsgearhover_png, BinaryData::settingsgearhover_pngSize, p.sizePortion),
      closeSettingsLabel("", ""),
     licenseChecker(p.licenseManager, p)
{
    sizePortion = p.sizePortion;
    if (juce::JUCEApplicationBase::isStandaloneApp()) {
        
        addAndMakeVisible(settingsButton);
        settingsButton.addListener(this);
//        if (auto* pluginHolder = juce::StandalonePluginHolder::getInstance())
//        {
//          pluginHolder->getMuteInputValue().setValue(false);
//        }
    }
    models = audioProcessor.getModels();
    state = valueTreeState.state;
    
    startTimer (10);
    addAndMakeVisible(inMeter);
    inMeterLabel.attachToComponent(&inMeter, true);
    
    addAndMakeVisible(outMeterL);
    outMeterLLabel.attachToComponent(&outMeterL, true);

    addAndMakeVisible(inClip);
    addAndMakeVisible(outClipL);
    
    addAndMakeVisible(cc);
    resizeButton.addListener(this);
    addAndMakeVisible(resizeButton);
    
    // Add the overlay and hide it initially
    addAndMakeVisible(overlay);
    overlay.setVisible(audioProcessor.licenseVisibility); // Hidden by default
    
    addAndMakeVisible(licenseChecker);
    licenseChecker.setLookAndFeel(&customLookAndFeel1);
    addAndMakeVisible(licenseButton);
    licenseButton.addListener(this);
    licenseButton.setLookAndFeel(new NoOutlineLookAndFeel());
    licenseChecker.setVisible(audioProcessor.licenseVisibility.load());
    setSize (sizePortion*fullWidth, sizePortion*fullHeight);
}

void EqAudioProcessorEditor::buttonClicked(juce::Button *button) {
    if (button == &settingsButton) {
        // Access the StandalonePluginHolder
        StandalonePluginHolder* pluginHolder = juce::StandalonePluginHolder::getInstance();

        // Get the AudioDeviceManager
        auto& deviceManager = pluginHolder->deviceManager;

        // Create a custom settings component that includes the mute button
        class CustomSettingsComponent : public juce::Component
        {
        public:
            CustomSettingsComponent(juce::AudioDeviceManager& dm, StandalonePluginHolder& holder, juce::LookAndFeel* laf)
                : deviceManager(dm), pluginHolder(holder),
                  deviceSelector(dm, 0, 2, 0, 2, false, false, false, false),
                  muteLabel("", "Mute Input:"),
                  muteButton("Mute audio input to avoid feedback")
            {
                addAndMakeVisible(deviceSelector);
                addAndMakeVisible(muteLabel);
                addAndMakeVisible(muteButton);
                
                // Apply custom look and feel to match the rest of the interface
                if (laf != nullptr)
                {
                    setLookAndFeel(laf);
                    muteButton.setLookAndFeel(laf);
                    muteLabel.setLookAndFeel(laf);
                    deviceSelector.setLookAndFeel(laf);
                }
                
                muteButton.setClickingTogglesState(true);
                muteButton.getToggleStateValue().referTo(pluginHolder.getMuteInputValue());
                
                // Ensure the settings are saved when the value changes
                muteButton.onClick = [&holder = pluginHolder]()
                {
                    holder.saveAudioDeviceState();
                };
                
                muteLabel.attachToComponent(&muteButton, true);
                setSize(500, 400);
            }
            
            void resized() override
            {
                auto bounds = getLocalBounds();
                auto muteArea = bounds.removeFromTop(40);
                muteButton.setBounds(muteArea.removeFromRight(300).reduced(5));
                bounds.removeFromTop(10); // spacing
                
                // Set device selector to take remaining space and size itself properly
                deviceSelector.setBounds(bounds);
                
                // Resize the component to fit content better
                auto idealHeight = 40 + 10 + deviceSelector.getHeight();
                if (idealHeight < getHeight())
                    setSize(getWidth(), idealHeight);
            }
            
        private:
            juce::AudioDeviceManager& deviceManager;
            StandalonePluginHolder& pluginHolder;
            juce::AudioDeviceSelectorComponent deviceSelector;
            juce::Label muteLabel;
            juce::ToggleButton muteButton;
        };

        auto* audioSettingsComp = new CustomSettingsComponent(deviceManager, *pluginHolder, &customLookAndFeel1);
        
        // Create a dialog window to display the settings
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(audioSettingsComp); // Attach the settings component
        options.dialogTitle = TRANS("Settings"); // Set dialog title
        juce::Colour bgColour((uint8)0x05, (uint8)0x05, (uint8)0x05, (float)1.0f);
        options.dialogBackgroundColour = bgColour;
        options.escapeKeyTriggersCloseButton = true; // Close on escape key
        options.useNativeTitleBar = true; // Use native title bar
        options.resizable = false; // Non-resizable dialog

        // Launch the dialog asynchronously
        options.launchAsync();
    }
    else if (button == &resizeButton) {
        if (sizePortion == 1.0) {
            sizePortion = 0.75;
        }
        else if (sizePortion == 0.75) {
            sizePortion = 1.0;
        }
        setProcessorSizePortion(sizePortion);
        cc.sizePortion = sizePortion;
        licenseChecker.sizePortion = sizePortion;
        setSize((int)fullWidth*sizePortion, (int)fullHeight*sizePortion);
        licenseChecker.updateLicenseFieldsWithResize(sizePortion);
        resized();
        repaint();
        return;
    }
    else if (button == &licenseButton) {
        if (!audioProcessor.licenseVisibility.load())
        {
            showLicensePage(); // Show license page and overlay
            audioProcessor.licenseVisibility.store(true);
        }
        else
        {
            audioProcessor.licenseVisibility.store(false);
            hideLicensePage(); // Hide license page and overlay
        }
    }
}

EqAudioProcessorEditor::~EqAudioProcessorEditor()
{
    stopTimer();
    settingsButton.removeListener(this);
    resizeButton.removeListener(this);
}

//==============================================================================
void EqAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    background = juce::ImageCache::getFromMemory(BinaryData::InvaderBackground_V2_png, BinaryData::InvaderBackground_V2_pngSize);
    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
    bgLight = juce::ImageCache::getFromMemory(BinaryData::Invaderbglight_V2_png, BinaryData::Invaderbglight_V2_pngSize);
    g.drawImageWithin(bgLight, (getWidth()-sizePortion*bgLight.getWidth())/2, (getHeight()-sizePortion*bgLight.getHeight())/2, sizePortion*bgLight.getWidth(), sizePortion*bgLight.getHeight(), juce::RectanglePlacement::stretchToFit);
    mainDial = juce::ImageCache::getFromMemory(BinaryData::buttonbig_png, BinaryData::buttonbig_pngSize);
    g.drawImageWithin(mainDial, 287*sizePortion, 309.5*sizePortion, sizePortion*mainDial.getWidth(), sizePortion*mainDial.getHeight(), juce::RectanglePlacement::stretchToFit);
    inGainDial = juce::ImageCache::getFromMemory(BinaryData::rotary_png, BinaryData::rotary_pngSize);
    g.drawImageWithin(inGainDial, 131*sizePortion, 104*sizePortion, sizePortion*inGainDial.getWidth(), sizePortion*inGainDial.getHeight(), juce::RectanglePlacement::stretchToFit);
    outGainDial = juce::ImageCache::getFromMemory(BinaryData::rotary_png, BinaryData::rotary_pngSize);
    g.drawImageWithin(outGainDial, 803*sizePortion, 104*sizePortion, sizePortion*outGainDial.getWidth(), sizePortion*outGainDial.getHeight(), juce::RectanglePlacement::stretchToFit);
}

void EqAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    double width = 980.0;
    double height = 980.0;
    cc.setBounds(getLocalBounds());
    licenseChecker.setBoundsRelative(0.3, 0.4, 0.4, 0.2);
    licenseChecker.resized();
    licenseButton.setBoundsRelative(0.4, 0.845, 0.195, 0.07);
    inMeter.setBoundsRelative(116.1/width, 115.0/height, 5.0/width, 38.5/height);
    inClip.setBoundsRelative(116.1/width, 110.0/height, 5.0/width, 3.0/height);
    outMeterL.setBoundsRelative(867.9/width, 115.0/height, 5.0/width, 38.5/height);
    outClipL.setBoundsRelative(867.9/width, 110.0/height, 5.0/width, 3.0/height);
    resizeButton.setBoundsRelative(914.0/width, 921.5/height, 56.0/height, 55.0/height);
    settingsButton.setBoundsRelative(13.5/width, 921.5/height, 56.0/height, 55.0/height);
    overlay.setBounds(getLocalBounds()); // Make the overlay cover the entire UI
    cc.toBack();
//    inMeter.toFront (false);  // inMeter stays on top of cc
//    outMeterL.toFront (false);
//    inClip.toFront (false);
//    outClipL.toFront (false);
}

void EqAudioProcessorEditor::updateToggleState(juce::Button* button) {
    auto state = button->getToggleState();
    if (button == &centHzButton) {
        audioProcessor.setCentHz(state);
    }
    else if (button == &muteButton) {
        audioProcessor.setMuted(state);
    }
}

void EqAudioProcessorEditor::showLicensePage()
{
    overlay.setVisible(true);             // Show the dark overlay
    licenseChecker.setVisible(true);      // Show the license checker
    licenseChecker.toFront(true);         // Bring the license checker to the front
}

void EqAudioProcessorEditor::hideLicensePage()
{
    overlay.setVisible(false);            // Hide the dark overlay
    licenseChecker.setVisible(false);     // Hide the license checker
}
