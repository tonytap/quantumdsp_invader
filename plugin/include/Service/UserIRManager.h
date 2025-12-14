#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include "../../dsp/ImpulseResponse.h"

namespace Service {

struct UserIRData {
    juce::String name;
    juce::String path;
    std::shared_ptr<dsp::ImpulseResponse> ir;
};

class UserIRManager {
public:
    UserIRManager();
    ~UserIRManager();
    
    bool loadUserIRsFromDirectory(const juce::String& directoryPath, double sampleRate = 48000.0);
    
    void clearUserIRs();
    
    bool resampleUserIRs(double newSampleRate);
    
    bool validateIRFile(const juce::File& irFile) const;
    
    std::shared_ptr<dsp::ImpulseResponse> getUserIR(int index) const;
    
    void populateComboBox(juce::ComboBox& comboBox) const;
    
    const std::vector<UserIRData>& getUserIRs() const { return mUserIRs; }
    
    size_t getNumUserIRs() const { return mUserIRs.size(); }
    
    juce::StringArray getUserIRNames() const;
    
    bool isEmpty() const { return mUserIRs.empty(); }
    
    void setDefaultSampleRate(double sampleRate) { mDefaultSampleRate = sampleRate; }
    
private:
    std::vector<UserIRData> mUserIRs;
    std::vector<UserIRData> mOriginalUserIRs;
    double mDefaultSampleRate;
    
    void storeOriginalIRs();
    std::shared_ptr<dsp::ImpulseResponse> createIRFromFile(const juce::String& filePath, double sampleRate);
    bool isValidWavFile(const juce::File& file) const;
};

} // namespace Service