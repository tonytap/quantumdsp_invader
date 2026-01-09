#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include "../defines.h"
#include "Gui/PresetPanel.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

enum class State
{
    Off,
    On,
    StayOn
};

typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
inline float titleSize = 40.f;
inline float valueSize = 28.f;
inline float ioSize = 15.f;
//inline float dropdownFontSize = 15.f;
inline std::array<float, 20> randomAngles = {
    0.000578, 0.000281, -0.0013, 0.001745, 0.001165,
    -0.000085, -0.000933, 0.001833, -0.00059, 0.000059,
    0.001931, 0.00138, 0.000101, 0.000869, 0.000742,
    0.001021, 0.000394, -0.001475, 0.000636, -0.000989
};
inline std::array<float, 40> opacityValues = {
    0.95f, 0.84257441f, 0.74729647f, 0.66279251f, 0.58784422f,
    0.52137105f, 0.46241464f, 0.410125f, 0.36374824f, 0.32261575f,
    0.2861345f, 0.25377854f, 0.22508137f, 0.19962927f, 0.17705528f,
    0.15703394f, 0.13927661f, 0.12352728f, 0.10955886f, 0.09717f,
    0.08618206f, 0.07643663f, 0.06779321f, 0.06012718f, 0.05332802f,
    0.04729771f, 0.04194931f, 0.0372057f, 0.0329985f, 0.02926704f,
    0.02595754f, 0.02302227f, 0.02041892f, 0.01810996f, 0.01606209f,
    0.0142458f, 0.01263489f, 0.01120614f, 0.00993896f, 0.00881506f
};
inline std::array<float, 40> eq1Values = {
    0.012, 0.012742038558544316, 0.013529962218952508, 0.014366608357461723, 0.015254989803856858, 0.01619830569091204, 0.01719995297472408, 0.018263538667423605, 0.019392892826314723, 0.0205920823462183, 0.021865425604686108, 0.023217508012824378, 0.024653198527726652, 0.026177667185978415, 0.027796403721373097, 0.029515237333883393, 0.031340357681077416, 0.03327833716757157, 0.03533615461278629, 0.037521220382233864, 0.039841403072838566, 0.04230505784838459, 0.04492105652713034, 0.04769881952993697, 0.05064834980395863, 0.053780268844056775, 0.05710585494165425, 0.0606370837987664, 0.06438667165346369, 0.06836812107206389, 0.07259576957295534, 0.0770848412571496, 0.081851501631489, 0.08691291582193214, 0.09228731038654797, 0.09799403895081182, 0.10405365190156148, 0.11048797039058698, 0.11732016491434685, 0.12457483875278212
};

inline void makeDisplayString(const juce::String ID, float val, juce::AttributedString *str, juce::AttributedString *strVal = nullptr) {
    str->clear();
    strVal->clear();

    str->append(ID);

    if (ID == "GAIN 1\n" || ID == "GAIN 2\n") {
        strVal->append(juce::String(val, 1));//, valueFont);
    }
    else if (ID == "REVERB\n" || ID == "DELAY\n") {
        strVal->append(juce::String((int)(val * 100)) + "%");//, valueFont);
    }
    else if (ID == "GATE\n") {
        if (val < -99.9) {
            strVal->append("OFF");//, valueFont);
        } else {
            strVal->append(juce::String(val, 1) + " dB");//, valueFont);
        }
    }
    else {
        strVal->append(juce::String(val, 1) + " dB");//, valueFont);
    }
}

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
        setColour(juce::ComboBox::backgroundColourId, bgColour);
        setColour(juce::PopupMenu::backgroundColourId, bgColour);
        setColour(juce::TextButton::buttonColourId, bgColour);
        setColour(juce::ListBox::backgroundColourId, bgColour);
        setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
    }
    // Override to set the combo box item text font
    juce::Font getComboBoxFont(juce::ComboBox& comboBox) override
    {
        float comboBoxHeight = comboBox.getHeight();
        return juce::Font("Arial", comboBoxHeight*0.6f, juce::Font::plain);
    }
    // Override to set the font for all items in the ComboBox dropdown
    juce::Font getPopupMenuFont() override
    {
        return juce::Font("Arial", comboBoxHeight*0.6f, juce::Font::plain); // Set font to Arial, size 14 for dropdown items
    }
    // Override to set the font for button text
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font("Arial", buttonHeight*0.6f, juce::Font::plain); // Set font to Arial, size 14
    }
    float comboBoxHeight;
    // Override to set the font and style for ToggleButton components
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        // Set the font for the toggle button text
        g.setFont(juce::Font("Arial", dropdownFontSize, juce::Font::plain)); // Set font to Arial, size 14

        // Draw the JUCE default ToggleButton style
        juce::LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
};
class CustomLookAndFeel1 : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel1()
    {
        juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
        setColour(juce::ComboBox::backgroundColourId, bgColour);
        setColour(juce::PopupMenu::backgroundColourId, bgColour);
        setColour(juce::TextButton::buttonColourId, bgColour);
        setColour(juce::ListBox::backgroundColourId, bgColour);
    }
    // Override to set the combo box item text font
    juce::Font getComboBoxFont(juce::ComboBox& comboBox) override
    {
        comboBoxHeight = comboBox.getHeight();
        return juce::Font("Arial", comboBoxHeight*0.6f, juce::Font::plain);
    }
    // Override to set the font for all items in the ComboBox dropdown
    juce::Font getPopupMenuFont() override
    {
        return juce::Font("Arial", comboBoxHeight*0.6f, juce::Font::plain); // Set font to Arial, size 14 for dropdown items
    }
    // Override to set the font for button text
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font("Arial", buttonHeight*0.6f, juce::Font::plain); // Set font to Arial, size 14
    }
    juce::Font getLabelFont(juce::Label& label) override
    {
        // Return a custom font with size and style
        return juce::Font("Arial", 16.f, juce::Font::plain);
    }
    // Custom method to get the font for ToggleButton
    juce::Font getToggleButtonFont(const juce::ToggleButton& button, int buttonHeight)
    {
        return juce::Font("Arial", buttonHeight * 0.6f, juce::Font::plain); // Adjust font as needed
    }
    float comboBoxHeight;
};
class CustomButton : public juce::Button
{
public:
    // ID is the dsp parameter for one click button
    // ID1, ID2 are the dsp parameters for two click buttons
    CustomButton(juce::AudioProcessorValueTreeState& vts, const void* buttonOnImg, int buttonOnImgSize, const void* buttonOffImg, int buttonOffImgSize, const juce::String& sID, const juce::String& ID = "n/a", const juce::String& ID1 = "n/a", const juce::String& ID2 = "n/a")
    : juce::Button("button"), stateID(sID), buttonID(ID), paramID1(ID1), paramID2(ID2), lastUsedID(paramID1), valueTreeState(vts)
    {
        onImage = juce::ImageFileFormat::loadFrom(buttonOnImg, buttonOnImgSize);
        offImage = juce::ImageFileFormat::loadFrom(buttonOffImg, buttonOffImgSize);
    }
    void turnOff()
    {
        currentState = State::Off;
        repaint();
        valueTreeState.getParameterAsValue(stateID).setValue(false);
    }
    void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        if (currentState == State::Off)
        {
            g.drawImage(offImage, bounds);
        }
        else
        {
            g.drawImage(onImage, bounds);
        }
    }
    int getWidth() {
        return onImage.getWidth();
    }
    int getHeight() {
        return onImage.getHeight();
    }
    juce::String buttonID;
    juce::String paramID1;
    juce::String paramID2;
    juce::String lastUsedID;
    juce::String stateID;
    
    State currentState = State::Off;
protected:
    juce::Image onImage, offImage;
private:
    juce::AudioProcessorValueTreeState& valueTreeState;
};

class CustomRotarySlider : public juce::Slider
{
public:
    CustomRotarySlider(const void* img, int imgSize, EqAudioProcessor& p, juce::AudioProcessorValueTreeState& vts, Gui::PresetPanel* pp, juce::Label* pl, juce::Label* pvl = nullptr, bool isIo = true)
    : juce::Slider(), audioProcessor(p), valueTreeState(vts), irDropdown (p.getIRDropdown())
    {
        setSliderStyle(juce::Slider::Rotary);
        setTextBoxStyle(NoTextBox, false, 0, 0);
        knobImage = juce::ImageFileFormat::loadFrom(img, imgSize);
        presetPanel = pp;
        parameterLabel = pl;
        parameterValueLabel = pvl;
        isIoKnob = isIo;
    }
    void setAngles(double start, double end)
    {
        startAngle = start*juce::MathConstants<double>::pi;
        endAngle = end*juce::MathConstants<double>::pi;
    }
    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        // Calculate scaling factor based on the current bounds size vs original image size
        float scaleX = (float)bounds.getWidth() / knobImage.getWidth();
        float scaleY = (float)bounds.getHeight() / knobImage.getHeight();
        auto lightBarBounds = bounds.reduced (0.093f * bounds.getWidth());
        double val = getValue();
        double max = getMaximum();
        double min = getMinimum();
        double angleRotated;
        angleRotated = ((val-min) / (max-min))*(endAngle-startAngle);
        if (ID == "ir selection" || ID == "preset selection") {
            setAngles(0.0, 2.0);
            if (val == max) {
                angleRotated = 0;
            }
        }
        else {
            setAngles(-0.75, 0.75);
            angleRotated = ((val-min) / (max-min))*(endAngle-startAngle);
            g.setColour(Constants::lightColour);
            float eq1 = 0.96;
            if (ID == "input gain" || ID == "output gain") {
                eq1 = 0.9;
            }
            juce::Random random;
            // Draw glow effect around the arc by creating multiple layers of slightly larger arcs
            for (int i = 0; i < 40; ++i) {  // Number of glow layers
                float opacity = opacityValues[i]; // Exponential decay for opacity
                float gloweq1 = eq1 + eq1Values[i]; // Increase eq1 for each layer
                juce::Colour glowColour = Constants::lightColour.withAlpha(opacity); // Adjust opacity for glow

                g.setColour(glowColour);
                Path glowArc;
                glowArc.addPieSegment(lightBarBounds, startAngle, startAngle + angleRotated, gloweq1);
                g.fillPath(glowArc);
            }
            // Draw the main arc path
            g.setColour(Constants::lightColour);
            Path filledArc1;
            filledArc1.addPieSegment(lightBarBounds, startAngle, startAngle + angleRotated, eq1);
            g.fillPath(filledArc1);
        }

        // Apply scaling and rotation
        juce::AffineTransform transform = juce::AffineTransform::scale(scaleX, scaleY)
            .followedBy(juce::AffineTransform::rotation(startAngle + angleRotated, centerX, centerY));
        g.drawImageTransformed(knobImage, transform, false);
    }
    int getWidth() {
        return knobImage.getWidth();
    }
    int getHeight() {
        return knobImage.getHeight();
    }
    void valueChanged() override
    {
        DBG("=== CustomRotarySlider::valueChanged() called ===");
        DBG("  ID: " << ID);
        DBG("  recalledFromPreset: " << (int)audioProcessor.recalledFromPreset);
        DBG("  value: " << getValue());
        double val = getValue();
        if (ID == "amp gain" || ID == "boost gain") {
            valueTreeState.getParameterAsValue("amp gain").setValue(val);
            audioProcessor.setAmp();
        }
        else if (ID == "ir selection") {
            irDropdown.setSelectedId(((int)val%(Constants::NUM_IRS+1))+1);
        }
        else if (ID == "preset selection") {
            int snappedValue = (int)val%((int)(getMaximum()-getMinimum()));
            setValue(snappedValue, juce::dontSendNotification);
        }
        if ((ID == "input gain" || ID == "output gain")/* && !audioProcessor.recalledFromPreset*/) {
            parameterLabel->setText(juce::String(val, 1)+" dB", juce::dontSendNotification);
        }
        else {
            juce::AttributedString paramLabelStr;
            juce::AttributedString paramLabelStrVal;
            if (ID == "reverb") {
                makeDisplayString("REVERB\n", val, &paramLabelStr, &paramLabelStrVal);
            }
            else if (ID == "delay mix") {
                makeDisplayString("DELAY\n", val, &paramLabelStr, &paramLabelStrVal);
            }
            else if (ID == "eq1") {
                makeDisplayString(Constants::eq1display, val, &paramLabelStr, &paramLabelStrVal);
            }
            else if (ID == "eq2") {
                makeDisplayString(Constants::eq2display, val, &paramLabelStr, &paramLabelStrVal);
            }
            else if (ID == "noise gate") {
                makeDisplayString("GATE\n", val, &paramLabelStr, &paramLabelStrVal);
            }
            else if (ID == "amp gain") {
                bool ampState = valueTreeState.getParameterAsValue("is amp 1").getValue();
                if (ampState) {
                    makeDisplayString("GAIN 1\n", val, &paramLabelStr, &paramLabelStrVal);
                }
                else {
                    makeDisplayString("GAIN 2\n", val, &paramLabelStr, &paramLabelStrVal);
                }
            }
            juce::String str = paramLabelStr.getText();
            juce::String valStr = paramLabelStrVal.getText();
            DBG("  str from makeDisplayString: " << str.toRawUTF8());
            DBG("  recalledFromPreset check: " << (int)(!audioProcessor.recalledFromPreset));
            if (!audioProcessor.recalledFromPreset) {
                DBG("  Setting label to: " << str.toRawUTF8());
                parameterLabel->setText(str, juce::dontSendNotification);
                parameterValueLabel->setText(valStr, juce::dontSendNotification);
                DBG("  Label after setting: " << parameterLabel->getText().toRawUTF8());
            }
            audioProcessor.recalledFromPreset = false;
        }
    }
    juce::String ID;
private:
    juce::Image knobImage;
    double startAngle;// = -0.75f * juce::MathConstants<float>::pi;
    double endAngle;// = 0.75f * juce::MathConstants<float>::pi;
    EqAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::ComboBox& irDropdown;
    Gui::PresetPanel* presetPanel = nullptr;
    juce::Label* parameterLabel = nullptr;
    juce::Label* parameterValueLabel = nullptr;
    bool isIoKnob;
};

class CentralComponents : public Component, ComboBox::Listener//, public juce::Timer
{
public:
    CentralComponents(const void* knobImg, int knobImgSize, EqAudioProcessor& p, juce::AudioProcessorValueTreeState& vts, const void* knobImg2, int knobImg2Size)
    : ampGainButton(vts, BinaryData::buttongainon_png, BinaryData::buttongainon_pngSize, BinaryData::buttongainoff_png, BinaryData::buttongainoff_pngSize, "gain state", "gain", "amp gain", "amp gain"),
    eqButton(vts, BinaryData::button_eq_on_png, BinaryData::button_eq_on_pngSize, BinaryData::button_eq_off_png, BinaryData::button_eq_off_pngSize, "eq state", "eq", "eq1", "eq2"),
    irButton(vts, BinaryData::button_ir_on_png, BinaryData::button_ir_on_pngSize, BinaryData::button_ir_off_png, BinaryData::button_ir_off_pngSize, "ir state", "ir", "ir selection", "ir selection"),
    gateButton(vts, BinaryData::buttongateon_png, BinaryData::buttongateon_pngSize, BinaryData::buttongateoff_png, BinaryData::buttongateoff_pngSize, "gate state", "gate", "noise gate", "noise gate"),
    fxButton(vts, BinaryData::button_fx_on_png, BinaryData::button_fx_on_pngSize, BinaryData::button_fx_off_png, BinaryData::button_fx_off_pngSize, "reverb state", "reverb", "reverb", "delay mix"),
    valueTreeState(vts),
    irDropdown (p.getIRDropdown()),
    userIRDropdown (p.getCustomIRDropdown()),
    audioProcessor(p),
    p1Button(vts, BinaryData::button_P1_on_png, BinaryData::button_P1_on_pngSize, BinaryData::button_P1_off_png, BinaryData::button_P1_off_pngSize, "p1 state", "p1", "preset selection", "preset selection"),
    p2Button(vts, BinaryData::button_P2_on_png, BinaryData::button_P2_on_pngSize, BinaryData::button_P2_off_png, BinaryData::button_P2_off_pngSize, "p2 state", "p2", "preset selection", "preset selection"),
    p3Button(vts, BinaryData::button_P3_on_png, BinaryData::button_P3_on_pngSize, BinaryData::button_P3_off_png, BinaryData::button_P3_off_pngSize, "p3 state", "p3", "preset selection", "preset selection"),
    p4Button(vts, BinaryData::button_P4_on_png, BinaryData::button_P4_on_pngSize, BinaryData::button_P4_off_png, BinaryData::button_P4_off_pngSize, "p4 state", "p4", "preset selection", "preset selection"),
    p5Button(vts, BinaryData::button_P5_on_png, BinaryData::button_P5_on_pngSize, BinaryData::button_P5_off_png, BinaryData::button_P5_off_pngSize, "p5 state", "p5", "preset selection", "preset selection"),
    presetPanel(p.getPresetManager(), p, &parameterLabel, &parameterValueLabel),
    mainKnob(knobImg, knobImgSize, p, vts, &presetPanel, &parameterLabel, &parameterValueLabel, false),
    inGainKnob(knobImg2, knobImg2Size, p, vts, &presetPanel, &inLabel),
    outGainKnob(knobImg2, knobImg2Size, p, vts, &presetPanel, &outLabel)
    {
        sizePortion = p.sizePortion;
        setComponentInfo(&ampGainButton);
        setComponentInfo(&gateButton);
        setComponentInfo(&eqButton);
        setComponentInfo(&irButton, &irDropdown, &userIRDropdown, &nextIRButton, &prevIRButton, &customIRButton);
        setComponentInfo(&fxButton);
        setPresetComponentInfo(&p1Button);
        setPresetComponentInfo(&p2Button);
        setPresetComponentInfo(&p3Button);
        setPresetComponentInfo(&p4Button);
        setPresetComponentInfo(&p5Button);
        addAndMakeVisible(mainKnob);
        addAndMakeVisible(inGainKnob);
        inGainKnob.ID = "input gain";
        outGainKnob.ID = "output gain";
        inGainKnobAttachment = std::make_unique<SliderAttachment>(valueTreeState, "input gain", inGainKnob);
        addAndMakeVisible(outGainKnob);
        outGainKnobAttachment = std::make_unique<SliderAttachment>(valueTreeState, "output gain", outGainKnob);
        addAndMakeVisible(irDropdown);
        addAndMakeVisible(userIRDropdown);
        configureTextButton(nextIRButton, ">");
        configureTextButton(prevIRButton, "<");
        configureTextButton(customIRButton, "Custom IR");
        prevIRButton.setLookAndFeel(&customLookAndFeel1);
        nextIRButton.setLookAndFeel(&customLookAndFeel1);
        customIRButton.setLookAndFeel(&customLookAndFeel1);
        juce::Colour bgColour((uint8)0x28, (uint8)0x28, (uint8)0x28, (float)1.0f);
        irBackground.setColour(juce::TextEditor::backgroundColourId, bgColour);  // Set background color
        addAndMakeVisible(irBackground);
        addAndMakeVisible(presetPanel);
        makeIRVisible(false);
        makePresetPanelVisible(false);
        addAndMakeVisible(parameterLabel);
        addAndMakeVisible(parameterValueLabel);
        addAndMakeVisible(inLabel);
        addAndMakeVisible(outLabel);
        inLabel.setJustificationType(juce::Justification::centred);
        outLabel.setJustificationType(juce::Justification::centred);
        parameterLabel.setJustificationType(juce::Justification::centredBottom);
        parameterValueLabel.setJustificationType(juce::Justification::centred);

        // Restore button state WITHOUT triggering user interaction logic
        // This runs on both first open and reopening to ensure consistent state

        // Restore bottom button (main controls)
        restoreButtonState();

        // Restore preset button (visual state only, no preset loading)
        int presetButtonIndex = audioProcessor.lastPresetButton;
        CustomButton* savedPresetButton = topButtons[presetButtonIndex];
        savedPresetButton->currentState = State::On;
        savedPresetButton->repaint();
        valueTreeState.getParameterAsValue(savedPresetButton->stateID).setValue(true);

        // Turn off other preset buttons
        for (CustomButton* button : topButtons) {
            if (button != savedPresetButton) {
                button->turnOff();
            }
        }

        irDropdown.setLookAndFeel(&customLookAndFeel1);
        userIRDropdown.setLookAndFeel(&customLookAndFeel1);
    }
    
    ~CentralComponents() {
        inGainKnobAttachment.reset();
        outGainKnobAttachment.reset();
        mainKnobAttachment.reset();
        audioProcessor.hasNotClosedGUI = false;
    }
    
    bool isEdited = true;
    
    void setButtonState(int lastBottomButton, int lastPresetButton, bool phantomSave = false) {
        isEdited = phantomSave;
        buttons[lastBottomButton]->triggerClick();
        topButtons[lastPresetButton]->triggerClick();
        audioProcessor.stateInformationSet.store(false);
    }
    
    juce::ComboBox& irDropdown;
    juce::ComboBox& userIRDropdown;
    juce::TextButton nextIRButton, prevIRButton;
    juce::TextButton customIRButton;
    
    void activateDropdown(CustomButton* button, bool isEdited = false) {
        if (button == &irButton) {
            return;
        }
        else if (button == &p1Button) {
            presetPanel.buttonSelected = 1;
            presetPanel.setPresetNum(audioProcessor.p1n, isEdited);
        }
        else if (button == &p2Button) {
            presetPanel.buttonSelected = 2;
            presetPanel.setPresetNum(audioProcessor.p2n, isEdited);
        }
        else if (button == &p3Button) {
            presetPanel.buttonSelected = 3;
            presetPanel.setPresetNum(audioProcessor.p3n, isEdited);
        }
        else if (button == &p4Button) {
            presetPanel.buttonSelected = 4;
            presetPanel.setPresetNum(audioProcessor.p4n, isEdited);
        }
        else if (button == &p5Button) {
            presetPanel.buttonSelected = 5;
            presetPanel.setPresetNum(audioProcessor.p5n, isEdited);
        }
    }
    
    // Helper: Derive currentMainKnobID from button + boolean parameters
    juce::String getMainKnobIDForButton(CustomButton* button) {
        if (button == &ampGainButton) {
            return "amp gain";
        }
        else if (button == &gateButton) {
            return "noise gate";
        }
        else if (button == &irButton) {
            return "ir selection";
        }
        else if (button == &eqButton) {
            bool isEq1 = valueTreeState.getRawParameterValue("is eq 1")->load() > 0.5f;
            return isEq1 ? "eq1" : "eq2";
        }
        else if (button == &fxButton) {
            bool isFx1 = valueTreeState.getRawParameterValue("is fx 1")->load() > 0.5f;
            return isFx1 ? "reverb" : "delay mix";
        }
        return "";
    }

    // Helper: Attach main knob to a parameter (used by both restoration and user clicks)
    void attachMainKnob(const juce::String& paramID) {
        audioProcessor.currentMainKnobID = paramID;
        mainKnob.ID = paramID;

        mainKnobAttachment.reset();
        mainKnobAttachment = std::make_unique<SliderAttachment>(valueTreeState, paramID, mainKnob);
        // ↑ This triggers valueChanged() which will set the label during user interactions

        mainKnob.setVisible(true);
        makeIRVisible(false);
    }

    // Restore state WITHOUT triggering user interaction logic
    void restoreButtonState() {
        int savedButtonIndex = audioProcessor.lastBottomButton;
        CustomButton* savedButton = buttons[savedButtonIndex];

        // Derive currentMainKnobID from saved button + boolean state
        audioProcessor.currentMainKnobID = getMainKnobIDForButton(savedButton);

        // Set button visual state (no click!)
        savedButton->currentState = State::On;
        savedButton->repaint();
        valueTreeState.getParameterAsValue(savedButton->stateID).setValue(true);

        // Store lastUsedID for the button
        savedButton->lastUsedID = audioProcessor.currentMainKnobID;

        // Turn off other buttons
        for (CustomButton* button : buttons) {
            if (button != savedButton) {
                button->turnOff();
            }
        }

        // Attach main knob (or show IR dropdown)
        if (savedButton == &irButton) {
            mainKnob.setVisible(false);
            makeIRVisible(true);
            juce::String irText = audioProcessor.lastTouchedDropdown->getText();
            parameterLabel.setText(irText, juce::dontSendNotification);
            parameterValueLabel.setText("", juce::dontSendNotification);
        } else {
            attachMainKnob(audioProcessor.currentMainKnobID);
        }

        DBG("RESTORED button: " << savedButton->buttonID << ", mainKnobID: " << audioProcessor.currentMainKnobID);
    }

    // For compatibility with PluginProcessor - used after setStateInformation
    void activateButton(CustomButton* button) {
        audioProcessor.recalledFromPreset = true;
        button->triggerClick();
        activateDropdown(button);
    }

    // For compatibility with PluginProcessor - used by setMainKnobID()
    void switchAttachmentTo() {
        // Derive which button should be active from currentMainKnobID
        int buttonIndex = 0;
        if (audioProcessor.currentMainKnobID == "amp gain") {
            buttonIndex = 0;
        }
        else if (audioProcessor.currentMainKnobID == "eq1" || audioProcessor.currentMainKnobID == "eq2") {
            buttonIndex = 1;
        }
        else if (audioProcessor.currentMainKnobID == "ir selection") {
            buttonIndex = 2;
        }
        else if (audioProcessor.currentMainKnobID == "noise gate") {
            buttonIndex = 3;
        }
        else if (audioProcessor.currentMainKnobID == "reverb" || audioProcessor.currentMainKnobID == "delay mix") {
            buttonIndex = 4;
        }

        audioProcessor.lastBottomButton = buttonIndex;
        activateButton(buttons[buttonIndex]);
    }
    
    void setMainKnobVal(double val) {
        mainKnob.setValue(val, juce::dontSendNotification);
    }
    juce::Label parameterLabel;
    juce::Label parameterValueLabel;
    juce::Label inLabel;
    juce::Label outLabel;
    double sizePortion = 1.0;
    Gui::PresetPanel presetPanel;
private:
    CustomButton fxButton, gateButton, ampGainButton, eqButton, irButton, p1Button, p2Button, p3Button, p4Button, p5Button;
    CustomRotarySlider mainKnob, inGainKnob, outGainKnob;
    juce::AudioProcessorValueTreeState& valueTreeState;
    EqAudioProcessor& audioProcessor;
    juce::Array<CustomButton*> buttons;
    juce::Array<CustomButton*> topButtons;
    std::unique_ptr<juce::FileChooser> irChooser;
    bool isStandalone = juce::JUCEApplicationBase::isStandaloneApp();
    CustomLookAndFeel customLookAndFeel;
    CustomLookAndFeel1 customLookAndFeel1;
    juce::TextEditor irBackground;
    
    std::unique_ptr<SliderAttachment> mainKnobAttachment, inGainKnobAttachment, outGainKnobAttachment;
    int cnt = 0;
    void mouseDown (const MouseEvent& e) override
    {
        // expand the slider’s hit‐area by N pixels:
        constexpr int padding = 10;
        if (inGainKnob.getBounds().expanded (padding).contains (e.getPosition())) {
            inGainKnob.mouseDown (e.getEventRelativeTo (&inGainKnob));
        }
        else if (outGainKnob.getBounds().expanded (padding).contains (e.getPosition())) {
            outGainKnob.mouseDown (e.getEventRelativeTo (&outGainKnob));
        }
    }
    // and likewise for mouseDrag/mouseUp...
    void comboBoxChanged(juce::ComboBox* dropdown) {
        juce::AttributedString irString;
        audioProcessor.lastTouchedDropdown = dropdown;
        if (dropdown == &irDropdown) {
            valueTreeState.state.setProperty("lastTouchedDropdown", true, nullptr);
            int selectedId = irDropdown.getSelectedId() - 1;
            irString.append(irDropdown.getItemText(selectedId), juce::Font("Impact", titleSize, juce::Font::plain));
            audioProcessor.getFactoryIR(selectedId);
            userIRDropdown.setSelectedId(userIRDropdown.getNumItems(), juce::dontSendNotification);
            int lastFactoryIR = irDropdown.getSelectedId();
            audioProcessor.valueTreeState.state.setProperty("lastIR", lastFactoryIR, nullptr);
            valueTreeState.getParameterAsValue("ir selection").setValue(selectedId);
        }
        else if (dropdown == &userIRDropdown) {
            valueTreeState.state.setProperty("lastTouchedDropdown", false, nullptr);
            int selectedId = userIRDropdown.getSelectedId() - 1;
            irString.append(userIRDropdown.getItemText(selectedId), juce::Font("Impact", titleSize, juce::Font::plain));
            audioProcessor.setCustomIR(selectedId);
            irDropdown.setSelectedId(irDropdown.getNumItems(), juce::dontSendNotification);
            int lastUserIR = irDropdown.getNumItems()+userIRDropdown.getSelectedId();
            audioProcessor.valueTreeState.state.setProperty("lastIR", lastUserIR, nullptr);
        }
        parameterLabel.setText(irString.getText(), juce::dontSendNotification);
//        if (!audioProcessor.recalledFromPreset) {
//            parameterLabel.setText(irString.getText(), juce::dontSendNotification);
//        }
    }
    
    void setComponentInfo(CustomButton* button, juce::ComboBox* ir = nullptr, juce::ComboBox* userIR = nullptr, juce::TextButton* next = nullptr, juce::TextButton* prev = nullptr, juce::TextButton* custom = nullptr)
    {
        addAndMakeVisible(*button);
        button->onClick = [this, button]() {handleButtonClick(button);};
        buttons.add(button);
        if (button == &irButton) {
            ir->onChange = [this, ir]() {comboBoxChanged(ir);};
            userIR->onChange = [this, userIR]() {comboBoxChanged(userIR);};
            next->onClick = [this, next]() {handleIRButtonClick(next);};
            prev->onClick = [this, prev]() {handleIRButtonClick(prev);};
            custom->onClick = [this, custom]() {handleIRButtonClick(custom);};
        }
    }
    juce::StringArray factoryIRs, allIRs;

    void handleIRButtonClick(juce::TextButton* button)
    {
        int nFacIR = irDropdown.getNumItems();
        int nUsrIR = userIRDropdown.getNumItems();
        int factoryIRId = irDropdown.getSelectedId();
        int userIRId = userIRDropdown.getSelectedId();
        if (button == &nextIRButton) {
            if (audioProcessor.lastTouchedDropdown == &irDropdown) {
                if (factoryIRId == nFacIR) {
                    userIRDropdown.setSelectedId(1);
                }
                else {
                    irDropdown.setSelectedId(factoryIRId+1);
                }
            }
            else {
                if (userIRId == nUsrIR) {
                    irDropdown.setSelectedId(1);
                }
                else {
                    userIRDropdown.setSelectedId(userIRId+1);
                }
            }
        }
        else if (button == &prevIRButton) {
            if (audioProcessor.lastTouchedDropdown == &irDropdown) {
                if (factoryIRId == 1) {
                    userIRDropdown.setSelectedId(nUsrIR);
                }
                else {
                    irDropdown.setSelectedId(factoryIRId-1);
                }
            }
            else {
                if (userIRId == 1) {
                    irDropdown.setSelectedId(nFacIR);
                }
                else {
                    userIRDropdown.setSelectedId(userIRId-1);
                }
            }
        }
        else if (button == &customIRButton) {
            irChooser = std::make_unique<juce::FileChooser> ("Select an Impulse Response File",
                                                             juce::File{},
                                                             "*.wav");
            auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            irChooser->launchAsync(chooserFlags,
                                   [this](const juce::FileChooser& fc)
                                   {
                juce::File file = fc.getResult();
                if (file != juce::File{}) {
                    dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
                    juce::String dspPath = file.getFullPathName();
                    
                    //populate custom IR dropdown
                    juce::StringArray customIRs = audioProcessor.loadUserIRsFromDirectory(dspPath);
                    audioProcessor.resampleUserIRs(audioProcessor.projectSr);
                    juce::String selectedFileName = file.getFileNameWithoutExtension();
                    for (int i = 0; i < customIRs.size() - 1; i++) { // -1 to exclude "Off"
                        if (customIRs[i] == selectedFileName) {
                            userIRDropdown.setSelectedId(i + 1);
                            auto raw_dspPath = dspPath.toRawUTF8();
                            audioProcessor.mStagedIR = audioProcessor.userIRs[i];
                            wavState = audioProcessor.mStagedIR->GetWavState();
                            audioProcessor.valueTreeState.state.setProperty("customIR", dspPath, nullptr);
                            break;
                        }
                    }
                    irDropdown.setSelectedId(irDropdown.getNumItems(), juce::dontSendNotification);
                    audioProcessor.updateAllIRs(customIRs);
                }
            });
        }
    }
    
    void setPresetComponentInfo(CustomButton* button)
    {
        addAndMakeVisible(*button);
        button->onClick = [this, button]() {handlePresetClick(button);};
        topButtons.add(button);
    }
    
    void handlePresetClick(CustomButton* clickedButton)
    {
        for (CustomButton* button : topButtons) {
            if (button != clickedButton) {
                button->turnOff();
                DBG(button->buttonID << " OFF");
            }
            else {
                DBG(button->buttonID << " ON");
                if (button->currentState == State::Off) {
                    button->currentState = State::On;
                    if (audioProcessor.recalledFromPreset) {
                    }
                    else {
                        activateDropdown(button);
                    }
                }
                else if (button->currentState == State::On) {
                    if (audioProcessor.recalledFromPreset) {
                    }
//                    else if (isStandalone) {
                    if (audioProcessor.justOpenedPreset) {
                        audioProcessor.justOpenedPreset = false;
                        activateDropdown(button, isEdited);
                    }
                    else {
                        audioProcessor.presetVisibility = !audioProcessor.presetVisibility;
                    }
//                    }
//                    else {
//                        if (audioProcessor.justOpenedPreset) {
//                            audioProcessor.justOpenedPreset = false;
//                            activateDropdown(button, isEdited);
//                        }
//                        else if (audioProcessor.hasNotClosedGUI) {
//                            audioProcessor.presetVisibility = !audioProcessor.presetVisibility;
//                        }
//                        else {
//                            audioProcessor.hasNotClosedGUI = true;
//                        }
//                    }
                }
                makePresetPanelVisible(audioProcessor.presetVisibility);
                if (button == &p1Button) {
                    audioProcessor.lastPresetButton = 0;
                }
                else if (button == &p2Button) {
                    audioProcessor.lastPresetButton = 1;
                }
                else if (button == &p3Button) {
                    audioProcessor.lastPresetButton = 2;
                }
                else if (button == &p4Button) {
                    audioProcessor.lastPresetButton = 3;
                }
                else if (button == &p5Button) {
                    audioProcessor.lastPresetButton = 4;
                }
                valueTreeState.state.setProperty("lastPresetButton", audioProcessor.lastPresetButton, nullptr);
                int b = valueTreeState.state.getProperty("lastPresetButton");
                DBG("from preset click, last is " << b);
            }
        }
    }

    void handleButtonClick(CustomButton* clickedButton)
    {
        for (CustomButton* button : buttons)
        {
            if (button != clickedButton)
            {
                button->turnOff();
            }
            else {
                audioProcessor.savedButtonID = button->buttonID;

                // Handle user clicks only (restoration uses restoreButtonState() instead)
                if (button->currentState == State::Off) {
                    // Button was off, turn it on
                    button->currentState = State::On;
                    // Derive currentMainKnobID from button + boolean
                    audioProcessor.currentMainKnobID = getMainKnobIDForButton(button);
                    button->lastUsedID = audioProcessor.currentMainKnobID;
                    DBG("USER CLICK (Off->On): button=" << button->buttonID << ", mainKnobID=" << audioProcessor.currentMainKnobID);
                }
                else if (button->currentState == State::On) {
                    // Button was already on - toggle between sub-parameters
                    if (button == &ampGainButton) {
                        // Toggle AMP model
                        bool ampState = valueTreeState.getParameterAsValue("is amp 1").getValue();
                        valueTreeState.getParameterAsValue("is amp 1").setValue(!ampState);
                        audioProcessor.currentMainKnobID = getMainKnobIDForButton(button);
                        button->lastUsedID = audioProcessor.currentMainKnobID;
                        DBG("TOGGLE AMP: is amp 1 = " << (int)!ampState << ", mainKnobID=" << audioProcessor.currentMainKnobID);
                    }
                    else if (button == &eqButton) {
                        // Toggle EQ parameter
                        bool isEq1 = valueTreeState.getParameterAsValue("is eq 1").getValue();
                        valueTreeState.getParameterAsValue("is eq 1").setValue(!isEq1);
                        audioProcessor.currentMainKnobID = getMainKnobIDForButton(button);
                        button->lastUsedID = audioProcessor.currentMainKnobID;
                        DBG("TOGGLE EQ: is eq 1 = " << (int)!isEq1 << ", mainKnobID=" << audioProcessor.currentMainKnobID);
                    }
                    else if (button == &fxButton) {
                        // Toggle FX parameter
                        bool isFx1 = valueTreeState.getParameterAsValue("is fx 1").getValue();
                        valueTreeState.getParameterAsValue("is fx 1").setValue(!isFx1);
                        audioProcessor.currentMainKnobID = getMainKnobIDForButton(button);
                        button->lastUsedID = audioProcessor.currentMainKnobID;
                        DBG("TOGGLE FX: is fx 1 = " << (int)!isFx1 << ", mainKnobID=" << audioProcessor.currentMainKnobID);
                    }
                }

                // Update UI - show IR dropdown or attach main knob
                if (button == &irButton) {
                    mainKnob.setVisible(false);
                    makeIRVisible(true);
                    juce::String irText = audioProcessor.lastTouchedDropdown->getText();
                    DBG("ir text = " << irText);
                    parameterLabel.setText(irText, juce::dontSendNotification);
                    parameterValueLabel.setText("", juce::dontSendNotification);
                }
                else {
                    attachMainKnob(audioProcessor.currentMainKnobID);
                    DBG("Main knob attached to: " << audioProcessor.currentMainKnobID);
                }

                // Update lastBottomButton
                if (button == &ampGainButton) audioProcessor.lastBottomButton = 0;
                else if (button == &gateButton) audioProcessor.lastBottomButton = 1;
                else if (button == &eqButton) audioProcessor.lastBottomButton = 2;
                else if (button == &irButton) audioProcessor.lastBottomButton = 3;
                else if (button == &fxButton) audioProcessor.lastBottomButton = 4;

                valueTreeState.state.setProperty("lastBottomButton", audioProcessor.lastBottomButton, nullptr);
            }
        }
    }
    
    void makeIRVisible(bool visibility) {
        nextIRButton.setVisible(visibility);
        prevIRButton.setVisible(visibility);
        customIRButton.setVisible(visibility);
        irDropdown.setVisible(visibility);
        userIRDropdown.setVisible(visibility);
        irBackground.setVisible(visibility);
    }
    
    void makePresetPanelVisible(bool visibility) {
        presetPanel.setVisible(visibility);
    }
    
    void configureTextButton(Button& button, const String& buttonText) {
        button.setButtonText(buttonText);
        button.setMouseCursor(MouseCursor::PointingHandCursor);
        addAndMakeVisible(button);
    }
    
    void resized() override
    {
        double width = 980.0;
        double height = 980.0;
        double mainKnobWP = (double)mainKnob.getWidth()/width;
        double mainKnobHP = (double)mainKnob.getHeight()/height;
        DBG("gain knob width: " << inGainKnob.getWidth());
        double gainKnobWP = (double)inGainKnob.getWidth()/width;
        DBG("gain knob height: " << inGainKnob.getHeight());
        double gainKnobHP = (double)inGainKnob.getHeight()/height;
        mainKnob.setBoundsRelative(242/980.0, 254/980.0, (double)mainKnob.getWidth()/width-0.003/*0.0009*/, (double)mainKnob.getHeight()/height);
        ampGainButton.setBoundsRelative(211.5/980.0, 746.5/980.0, (double)ampGainButton.getWidth()/width, (double)ampGainButton.getHeight()/height);
        gateButton.setBoundsRelative(326.5/980.0, 747.5/980.0, (double)gateButton.getWidth()/width, (double)gateButton.getHeight()/height);
        eqButton.setBoundsRelative(438.0/980.0, 745.0/980.0, (double)eqButton.getWidth()/width, (double)eqButton.getHeight()/height);
        irButton.setBoundsRelative(549.5/980.0, 747.0/980.0, (double)irButton.getWidth()/width, (double)irButton.getHeight()/height);
        fxButton.setBoundsRelative(662.5/980.0, 747.5/980.0, (double)fxButton.getWidth()/width, (double)fxButton.getHeight()/height);
        double irWP = 0.14;
        double irHP = 30.0/980.0;
        double irX = 549.5/980.0+(double)gateButton.getWidth()/width/2-0.11;
        irBackground.setBoundsRelative(irX, 0.62, 0.23, 0.13);
        prevIRButton.setBoundsRelative(irX+0.01, 0.63, 0.03, irHP);
        prevIRButton.setAlwaysOnTop(true);
        nextIRButton.setBoundsRelative(irX+0.045+irWP+0.005, 0.63, 0.03, irHP);
        nextIRButton.setAlwaysOnTop(true);
        irDropdown.setBoundsRelative(irX+0.045, 0.63, irWP, irHP);
        userIRDropdown.setBoundsRelative(irX+0.045, 0.67, irWP, irHP);
        userIRDropdown.setAlwaysOnTop(true);
        customIRButton.setBoundsRelative(irX+0.045, 0.71, irWP, irHP);
        customIRButton.setAlwaysOnTop(true);
        irDropdown.setAlwaysOnTop(true);
        irDropdown.repaint();
        p1Button.setBoundsRelative(212.0/980.0, 183.5/980.0, (double)p1Button.getWidth()/width, (double)p1Button.getHeight()/height);
        p2Button.setBoundsRelative(327.0/980.0, 185.5/980.0, (double)p2Button.getWidth()/width, (double)p2Button.getHeight()/height);
        p3Button.setBoundsRelative(437.5/980.0, 183/980.0, (double)p3Button.getWidth()/width, (double)p3Button.getHeight()/height);
        p4Button.setBoundsRelative(550.0/980.0, 185.0/980.0, (double)p4Button.getWidth()/width, (double)p4Button.getHeight()/height);
        p5Button.setBoundsRelative(663.0/980.0, 185.5/980.0, (double)p5Button.getWidth()/width, (double)p5Button.getHeight()/height);
        presetPanel.setBounds(getLocalBounds().removeFromTop(proportionOfHeight(0.05f)));
        parameterLabel.setBoundsRelative(0.4, 0.45, 0.2, 0.06);
        parameterLabel.setFont(juce::Font("Impact", sizePortion*titleSize, juce::Font::plain));
        parameterValueLabel.setBoundsRelative(0.4, 0.505, 0.2, 0.04);
        parameterValueLabel.setFont(juce::Font("Impact", sizePortion*valueSize, juce::Font::plain));
        inGainKnob.setBoundsRelative(132/980.0, 104/980.0, (double)inGainKnob.getWidth()/980.0, (double)inGainKnob.getHeight()/980.0);
        inLabel.setBoundsRelative(131/980.0, 125/980.0, 57/width, 0.1);
        inLabel.setFont(juce::Font(sizePortion*ioSize));
        outGainKnob.setBoundsRelative(804/980.0, 103.5/980.0, (double)outGainKnob.getWidth()/980.0, (double)outGainKnob.getWidth()/980.0);
        outLabel.setBoundsRelative(803/980.0, 125/980.0, 57/width, 0.1);
        outLabel.setFont(juce::Font(sizePortion*ioSize));
        inLabel.toBack();
        outLabel.toBack();
    }
};
