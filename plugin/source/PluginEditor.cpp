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
      cc(BinaryData::buttonbigdial_png, BinaryData::buttonbigdial_pngSize, p, vts, BinaryData::rotarydial_png, BinaryData::rotarydial_pngSize),
      resizeButton(BinaryData::resize_png, BinaryData::resize_pngSize, BinaryData::resizehover_png, BinaryData::resizehover_pngSize, p.sizePortion),
      settingsButton(BinaryData::settingsgear_png, BinaryData::settingsgear_pngSize, BinaryData::settingsgearhover_png, BinaryData::settingsgearhover_pngSize, p.sizePortion),
      closeSettingsLabel("", ""),
      overlay([this]() { hideLicensePage(); }, p),
      licenseChecker(p.licenseManager, p)
{
    sizePortion = p.sizePortion;
    if (juce::JUCEApplicationBase::isStandaloneApp()) {
        
        addAndMakeVisible(settingsButton);
        settingsButton.addListener(this);
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

    addAndMakeVisible(overlay);
    overlay.setVisible(audioProcessor.licenseVisibility.load());

    addAndMakeVisible(licenseChecker);
    licenseChecker.setLookAndFeel(&customLookAndFeel1);
    addAndMakeVisible(licenseButton);
    licenseButton.addListener(this);
    licenseButton.setLookAndFeel(&noOutlineLookAndFeel);
    licenseChecker.setVisible(audioProcessor.licenseVisibility.load());
    
    // Set default buffer size to 128 on first launch (standalone only)
    if (juce::JUCEApplicationBase::isStandaloneApp()) {
        auto& state = valueTreeState.state;
        if (!state.hasProperty("bufferSizeInitialized")) {
            if (auto* pluginHolder = juce::StandalonePluginHolder::getInstance()) {
                auto& deviceManager = pluginHolder->deviceManager;
                auto currentSetup = deviceManager.getAudioDeviceSetup();
                currentSetup.bufferSize = 128;
                deviceManager.setAudioDeviceSetup(currentSetup, true);
                state.setProperty("bufferSizeInitialized", true, nullptr);
            }
        }
    }

    // Register as listener to state changes
    valueTreeState.state.addListener(this);

    // Initialize UI from current state (works regardless of timing)
    refreshFromState();

    setSize (sizePortion*fullWidth, sizePortion*fullHeight);
}

void EqAudioProcessorEditor::setProportion() {
    setProcessorSizePortion(sizePortion);
    cc.sizePortion = sizePortion;
    licenseChecker.sizePortion = sizePortion;
    licenseChecker.updateLicenseFieldsWithResize(sizePortion);
    setSize((int)fullWidth*sizePortion, (int)fullHeight*sizePortion);
    resized();
    repaint();
}

void EqAudioProcessorEditor::buttonClicked(juce::Button *button) {
    if (button == &settingsButton) {
        // Access the StandalonePluginHolder
        StandalonePluginHolder* pluginHolder = juce::StandalonePluginHolder::getInstance();

        // Get the AudioDeviceManager
        auto& deviceManager = pluginHolder->deviceManager;

        // Create the AudioDeviceSelectorComponent
        auto* audioSettingsComp = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            0, 2,  // Min/max input channels
            0, 2,  // Min/max output channels
            false,  // Show input channels
            false,  // Show output channels
            false,  // Show sample rate selector
            false
        );

        audioSettingsComp->setLookAndFeel(&customLookAndFeel1);
        audioSettingsComp->setSize(500, 550);
        // Create a dialog window to display the settings
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(audioSettingsComp); // Attach the settings component
        options.dialogTitle = TRANS("Settings"); // Set dialog title
        options.dialogBackgroundColour = Constants::darkBackgroundColour;
        options.escapeKeyTriggersCloseButton = true; // Close on escape key
        options.useNativeTitleBar = true; // Use native title bar
        options.resizable = false; // Non-resizable dialog

        // Launch the dialog asynchronously
        options.launchAsync();
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
    else if (button == &resizeButton) {
        if (sizePortion == 1.0) {
            sizePortion = 0.75;
        }
        else if (sizePortion == 0.75) {
            sizePortion = 1.0;
        }
        valueTreeState.state.setProperty("size", sizePortion, nullptr);
        setProportion();
        return;
    }
}

EqAudioProcessorEditor::~EqAudioProcessorEditor()
{
    // Unregister state listener
    valueTreeState.state.removeListener(this);

    // Clear LookAndFeel references before destruction
    licenseChecker.setLookAndFeel(nullptr);
    licenseButton.setLookAndFeel(nullptr);

    stopTimer();
    settingsButton.removeListener(this);
    resizeButton.removeListener(this);
    licenseButton.removeListener(this);
}

//==============================================================================
void EqAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    background = juce::ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);
    g.drawImageWithin(background, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::stretchToFit);
    bgLight = juce::ImageCache::getFromMemory(BinaryData::bglight_png, BinaryData::bglight_pngSize);
    g.drawImageWithin(bgLight, (getWidth()-sizePortion*bgLight.getWidth())/2, (getHeight()-sizePortion*bgLight.getHeight())/2, sizePortion*bgLight.getWidth(), sizePortion*bgLight.getHeight(), juce::RectanglePlacement::stretchToFit);

    // Use different main dial image when IR button is active (lastBottomButton == 3)
    if (audioProcessor.lastBottomButton == 3) {
        mainDial = juce::ImageCache::getFromMemory(BinaryData::buttonbigir_png, BinaryData::buttonbigir_pngSize);
    } else {
        mainDial = juce::ImageCache::getFromMemory(BinaryData::buttonbig_png, BinaryData::buttonbig_pngSize);
    }
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
    inMeter.setBoundsRelative(116.1/width, 115.0/height, 5.0/width, 38.5/height);
    inClip.setBoundsRelative(116.1/width, 110.0/height, 5.0/width, 3.0/height);
    outMeterL.setBoundsRelative(867.9/width, 115.0/height, 5.0/width, 38.5/height);
    outClipL.setBoundsRelative(867.9/width, 110.0/height, 5.0/width, 3.0/height);
    resizeButton.setBoundsRelative(914.0/width, 921.5/height, 56.0/height, 55.0/height);
    settingsButton.setBoundsRelative(13.5/width, 921.5/height, 56.0/height, 55.0/height);
    licenseChecker.setBoundsRelative(0.3, 0.4, 0.4, 0.2);
    licenseChecker.resized();
    licenseButton.setBoundsRelative(0.4, 0.845, 0.195, 0.07);
    overlay.setBounds(getLocalBounds()); // Make the overlay cover the entire UI
    cc.toBack();
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

void EqAudioProcessorEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    // React to GUI button state changes only
    // IR selection (lastTouchedDropdown, customIR) is handled by restoreIRFromState() in processor
    if (property == juce::Identifier("lastBottomButton") ||
        property == juce::Identifier("lastPresetButton"))
    {
        refreshFromState();
    }
}

void EqAudioProcessorEditor::refreshFromState()
{
    // Update UI from current state (parameters are handled by attachments)
    auto state = valueTreeState.state;

    // Restore button states
    cc.restoreButtonState();

    // Restore preset button selection
    int presetButtonIndex = audioProcessor.lastPresetButton;
    if (presetButtonIndex >= 0 && presetButtonIndex < cc.topButtons.size()) {
        CustomButton* savedPresetButton = cc.topButtons[presetButtonIndex];
        savedPresetButton->currentState = State::On;
        savedPresetButton->repaint();
        valueTreeState.getParameterAsValue(savedPresetButton->stateID).setValue(true);

        // Turn off other preset buttons
        for (CustomButton* button : cc.topButtons) {
            if (button != savedPresetButton) {
                button->turnOff();
            }
        }
    }

    // IR restoration is handled by processor's restoreIRFromState()
}
