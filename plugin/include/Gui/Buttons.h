#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class CustomButton : public juce::Button
{
public:
    // ID is the dsp parameter for one click button
    // ID1, ID2 are the dsp parameters for two click buttons
    CustomButton(juce::AudioProcessorValueTreeState& vts, const void* buttonOnImg, int buttonOnImgSize, const void* buttonOffImg, int buttonOffImgSize, const juce::String& sID, const juce::String& ID = "n/a", const juce::String& ID1 = "n/a", const juce::String& ID2 = "n/a")
    : juce::Button("button"), stateID(sID), paramID(ID), paramID1(ID1), paramID2(ID2), lastUsedID(paramID1), valueTreeState(vts)
    {
        onImage = juce::ImageFileFormat::loadFrom(buttonOnImg, buttonOnImgSize);
        offImage = juce::ImageFileFormat::loadFrom(buttonOffImg, buttonOffImgSize);
//        attachment = std::make_unique<ButtonAttachment>(vts, stateID, *this);
//        bool state = valueTreeState.getParameterAsValue(stateID).getValue();
    }
//    ~CustomButton() {
//        attachment.reset();
//    }
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
    juce::String& getParamID() {
        return paramID;
    }
    juce::String paramID;
    juce::String paramID1;
    juce::String paramID2;
    juce::String lastUsedID;
    juce::String stateID;
    
    State currentState = State::Off;
    bool recalledFromPreset = false;
protected:
    juce::Image onImage, offImage;
private:
    juce::AudioProcessorValueTreeState& valueTreeState;
//    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};