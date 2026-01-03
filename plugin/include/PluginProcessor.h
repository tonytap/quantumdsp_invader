/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include "pn.h"
#include "shelf.h"
#include "reverb.h"
#include "delay.h"
#include "../NeuralAmpModelerCore/NAM/dsp.h"
#include "../NeuralAmpModelerCore/NAM/wavenet.h"
#include "../dsp/NoiseGate.h"
#include "../dsp/ResamplingContainer/ResamplingContainer.h"
#include <Eigen/Dense>
#include "../dsp/ImpulseResponse.h"
#include "Utility/ParameterHelper.h"
#include "Service/PresetManager.h"
#include <LicenseSpring/LicenseManager.h>
#include "AppConfig.h"

using namespace std;
using namespace juce;
#define BUFFERSIZE 32768
#define NUM_FACTORY_PRESETS 18
#define NUM_IRS 18
static StringArray factoryPresets = { "The Rocker", "Capt. Crunch", "Eh-I-See!", "Soaring Lead", "Cleaning Up", "Sweet Tea Blues", "Rokk", "Crisp & Clear", "Modern Singles", "Modern Rhythms", "Anger Management", "Psychedlica", "Flying Solo", "Texas Blooze", "Vibin'", "Dreamscape", "Thrasher", "Down Under" };

//==============================================================================
/**
*/
class EqAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    EqAudioProcessor();
    ~EqAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /*static*/ std::vector<std::shared_ptr<nam::DSP>> models;
    /*static*/ std::vector<std::shared_ptr<dsp::ImpulseResponse>> factoryIRs;
    std::vector<std::shared_ptr<dsp::ImpulseResponse>> originalFactoryIRs;

    std::atomic<bool> licenseVisibility {false};

    PeakNotch PN150;
    PeakNotch PN800;
    PeakNotch PN4k;
    PeakNotch GlobalEQ;
    Shelf HS5k;
    
    std::atomic<float>* thicknessParameter  = nullptr;
    std::atomic<float>* presenceParameter  = nullptr;
    struct SchroederReverb *Hall = initReverb(1.f, 0.f, 0.55f, 0.9f);
    std::vector<std::unique_ptr<Delay>> channelDelays;
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;
    std::atomic<float>* delayTimingParam = nullptr;
    std::shared_ptr<nam::DSP> amp1_dsp = nullptr;
    std::shared_ptr<nam::DSP> amp1_model = nullptr;
    std::shared_ptr<nam::DSP> old_model;
    Eigen::VectorXf mWeight;
    std::shared_ptr<dsp::ImpulseResponse> mIR = nullptr;
    std::shared_ptr<dsp::ImpulseResponse> mStagedIR = nullptr;
    juce::String p1n = factoryPresets[0];
    juce::String p2n = factoryPresets[1];
    juce::String p3n = factoryPresets[2];
    juce::String p4n = factoryPresets[3];
    juce::String p5n = factoryPresets[4];
    Service::PresetManager& getPresetManager() { return *presetManager; }
    bool ampOn = false;
    float ampGain = 1.f;
    bool isAmp1 = true;
    AudioProcessorValueTreeState valueTreeState;
    juce::ComboBox* lastTouchedDropdown = &irDropdown;
    void loadModel(const int amp_idx, double gainLvl, unsigned long i);
    void loadIR(const int i, double sampleRate = 48000.0);
    juce::StringArray loadUserIRsFromDirectory(const juce::String& customIRPath);
    juce::File writeBinaryDataToTempFile(const void* data, int size, const juce::String& fileName);
    std::tuple<std::unique_ptr<juce::XmlElement>, juce::File> writePresetBinaryDataToTempFile(const void* data, int size, const juce::String& fileName);
    const std::vector<std::shared_ptr<nam::DSP>>& getModels() const
    {
        return models;
    }
    void setPresetPath(const juce::String& newPath) { presetPath = newPath; }
    const juce::String& getPresetPath() const { return presetPath; }
    float getCurrentPitch() {return pitch;}
    void setStandardA(float val) {standard_A = val;}
    float getStandardA() {return standard_A;}
    juce::String getCurrentNote() {return noteName;}
    double getCurrentDeviation() {return dev;}
    void setCentHz(bool state) {isCent = state;}
    void setMuted(bool state) {isMuted = state;}
    void updateReferenceFrequencies() {
        for (int oct = -2; oct <= 1; oct++) {
            double octave_A = oct != 0 ? standard_A*pow(2, oct) : standard_A;
            for (int i = 0; i < 12; i++) {
                all_frequencies[(oct+2)*12+i] = octave_A*pow(2.0, ((double)i-9.0)/12.0);
            }
        }
    }
    void getFactoryIR(int i) {
        if (i < factoryIRs.size()) {
            mStagedIR = factoryIRs[i];
            irEnabled.store(true);
        }
        else {
            irEnabled.store(false);
        }
    }
    void setCustomIR(int i) {
        if (i < userIRs.size()) {
            mStagedIR = userIRs[i];
            irEnabled.store(true);
        } else {
            irEnabled.store(false);
        }
    }

    void enableSmoothing() {
        valueTreeState.getParameterAsValue("amp smooth").setValue(true);
    }
    void setModel() {
        amp1_dsp = models[model_id];
    }
    void setOldModel() {
        old_model = amp1_dsp;
    }
    float getInRMS();
    float getOutRMS(int ch);
    atomic<int> model_id = 0;
    atomic<int> ir_id = 0;
    void setAmp();
    juce::ComboBox& getIRDropdown() {
        return irDropdown;
    }
    juce::ComboBox& getCustomIRDropdown() {
        return userIRDropdown;
    }
    void populateIRDropdown();
    bool recallCustomIR = false;
    juce::StringArray facIRs, allIRs;
    void updateAllIRs(juce::StringArray& customIRs) {
        allIRs = facIRs;
        allIRs.addArray(customIRs);
    }
    void setIRName(int irN, int customIRN) {
        juce::StringArray customIRs;
        if (irN == irDropdown.getNumItems()) {
            juce::String dspPath (valueTreeState.state.getProperty("customIR"));
            customIRs = loadUserIRsFromDirectory(dspPath);
            userIRDropdown.setSelectedId(customIRN);
        }
        else {
            irDropdown.setSelectedId(irN);
        }
        updateAllIRs(customIRs);
    }
    void loadFactoryPresets(int i);
    std::vector<std::shared_ptr<dsp::ImpulseResponse>> userIRs;
    std::vector<std::shared_ptr<dsp::ImpulseResponse>> originalUserIRs;
    juce::ComboBox irDropdown;
    juce::ComboBox userIRDropdown;
    bool p1Switched = false;
    bool p2Switched = false;
    bool p3Switched = false;
    bool p4Switched = false;
    bool p5Switched = false;
    juce::String currentMainKnobID = "amp gain";
    void setMainKnobID();
    juce::String savedButtonID = "gain";
    bool recalledFromPreset = false;
    bool justOpenedBottom = true;
    bool justOpenedPreset = true;
    bool justOpenedGUI = true;
    bool hasNotClosedGUI = true;
    bool irVisibility = false;
    bool presetVisibility = false;
    void setMainKnobVal(double val);
    int lastPresetButton = 0;
    int lastBottomButton = 0;
    int lastPage2Button = 6;
    int lastPage1Button = 0;
    int lastDelayButton = 0;
    std::atomic<bool> presetSmoothing { false };
    std::atomic<bool> stateInformationSet { false };
    double sizePortion = 0.75;
    double projectSr = 48000.0;
    void resampleUserIRs(double projectSr);
    std::atomic<bool> licenseActivated { false };
    LicenseSpring::LicenseManager::ptr_t licenseManager;
private:
    std::atomic<float> smoothMix { 0.f };
    std::unique_ptr<Service::PresetManager> presetManager;
    //==============================================================================
    array<NAM_SAMPLE, BUFFERSIZE> dataIn = {};
    array<NAM_SAMPLE, BUFFERSIZE> dataOut = {};
    array<NAM_SAMPLE, BUFFERSIZE> crossfadeBuffer = {};
    double **irIn;
    juce::LinearSmoothedValue<float> inputGain {1.f};
    juce::LinearSmoothedValue<float> outputGain {1.f};
    juce::LinearSmoothedValue<float> hallWet {0.f};
    juce::LinearSmoothedValue<float> thicknessGain {0.f};
    juce::LinearSmoothedValue<float> presenceGain {0.f};
    unsigned long numModelFiles = 38;
    juce::String presetPath = "";
    std::shared_ptr<dsp::ImpulseResponse> identityIR;
    std::atomic<bool> irEnabled { false };
    bool isStandalone = juce::JUCEApplicationBase::isStandaloneApp();
    int fftSize;
    vector<float> in_buf;
    vector<float> orderedInBuf;
    unsigned int smpCnt;
    int inPtr;
    float pitch;
    float standard_A = 440.f;
    double sr;
    juce::String noPitch = "?";
    vector<float> acf;
    juce::String notes[12] = {"C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"};
    juce::String noteName = noPitch;
    vector<double> all_frequencies;
    vector<double> referenceFrequencies = {
        standard_A * std::pow(2.0, -9.0 / 12.0),  // C4
        standard_A * std::pow(2.0, -8.0 / 12.0),  // C#4/Db4
        standard_A * std::pow(2.0, -7.0 / 12.0),  // D4
        standard_A * std::pow(2.0, -6.0 / 12.0),  // D#4/Eb4
        standard_A * std::pow(2.0, -5.0 / 12.0),  // E4
        standard_A * std::pow(2.0, -4.0 / 12.0),  // F4
        standard_A * std::pow(2.0, -3.0 / 12.0),  // F#4/Gb4
        standard_A * std::pow(2.0, -2.0 / 12.0),  // G4
        standard_A * std::pow(2.0, -1.0 / 12.0),  // G#4/Ab4
        standard_A,                               // A4
        standard_A * std::pow(2.0, 1.0 / 12.0),   // A#4/Bb4
        standard_A * std::pow(2.0, 2.0 / 12.0),   // B4
    };
    vector<double> generateReferenceFrequencies()
    {
        vector<double> allFrequencies;
        for (int octave = -2; octave <= 1; ++octave)  // Covering a range of octaves suitable for guitar tuning
        {
            for (double baseFrequency : referenceFrequencies)
            {
                double freq = baseFrequency * std::pow(2.0, octave);
                allFrequencies.push_back(freq);
            }
        }
        return allFrequencies;
    }
    void setButtonState(int lastBottomButton, int lastPresetButton);
    double dev = 0;
    bool isCent = false;
    bool isMuted = false;
    void updateDeviation(double detectedFrequency)
    {
        double minDiff = std::numeric_limits<double>::max();
        double nearestFrequency = 0.0;
        for (size_t i = 0; i < all_frequencies.size(); ++i)
        {
            double diff = std::abs(all_frequencies[i] - detectedFrequency);
            if (diff < minDiff)
            {
                minDiff = diff;
                nearestFrequency = all_frequencies[i];
            }
        }
        dev = isCent ? 1200 * std::log2(detectedFrequency / nearestFrequency) : (detectedFrequency - nearestFrequency);
    }
    bool smoothDistortion = false;
    vector<float> interpBuf;
    unsigned int interpSmplCnt = 0;
    unsigned int interpSize;
    unsigned int presetInterpSmplCnt[2] = {0, 0};
    unsigned int presetInterpSize;
    float lastAmpOut = 0;
    dsp::ResamplingContainer<NAM_SAMPLE, 1, 12> mResampler1; // process current model
    dsp::ResamplingContainer<NAM_SAMPLE, 1, 12> mResampler2; // process old model
    dsp::ResamplingContainer<NAM_SAMPLE, 1, 12> irResampler; // process old model
    std::function<void(NAM_SAMPLE**, NAM_SAMPLE**, int)> setResamplingModelProcess (std::shared_ptr<nam::DSP> model)
    {
        // Capture the raw pointer by value:
        return [model] (NAM_SAMPLE** input,
                       NAM_SAMPLE** output,
                       int         numFrames)
        {
            // forward exactly as before:
            model->process (input[0],
                           output[0],
                           numFrames);
            model->finalize_(numFrames);
        };
    }
    std::function<double**(double**, int)> setResamplingIRProcess (std::shared_ptr<dsp::ImpulseResponse> ir)
    {
        // Capture the raw pointer by value:
        return [ir] (double** input,
                       int         numFrames)
        {
            // forward exactly as before:
            return ir->Process (input, 1, numFrames);
        };
    }
    std::function<void(std::shared_ptr<nam::DSP>)> mBlockProcessFunc;
    std::function<void(NAM_SAMPLE**, NAM_SAMPLE**, int)> resampleProcessFunc;
    void resampleFactoryIRs(double projectSr);
    
    double modelSr = 48000.0;
    
    juce::LinearSmoothedValue<float> rmsIn;
    juce::LinearSmoothedValue<float> rmsLeftOut;
    
    bool noiseGateActive = true;
    dsp::noise_gate::Trigger mNoiseGateTrigger;
    dsp::noise_gate::Gain mNoiseGateGain;
    
    float K, b0, b1, a0, a1, y1, x1, lpfMix;
    float mix;
    
    float *reverbWetL;
    float *reverbWetR;
    int reverbRp, reverbWp;
    
    double beatDivision = 1.0;
    float actualDelayMix = 0.f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqAudioProcessor)
};
