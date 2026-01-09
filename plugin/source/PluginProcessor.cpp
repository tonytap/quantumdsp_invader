/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "../include/PluginProcessor.h"
#include "../include/PluginEditor.h"
#include "../include/Service/PresetManager.h"
#include <cmath>

//==============================================================================
EqAudioProcessor::EqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
valueTreeState(*this, nullptr, "MLGuitarAmp", Utility::ParameterHelper::createParameterLayout()),
PN150(48000.0, 120),
PN800(48000.0, 800),
PN4k(48000.0, 2000),
HS5k(48000.0, 5000),
GlobalEQ(48000.0, 5200),
mResampler1(48000.0),
mResampler2(48000.0),
irResampler(48000.0)
#endif
{
    // Initialize preset button names with first 5 factory presets
    p1n = Constants::factoryPresets[0];
    p2n = Constants::factoryPresets[1];
    p3n = Constants::factoryPresets[2];
    p4n = Constants::factoryPresets[3];
    p5n = Constants::factoryPresets[4];

    valueTreeState.state.setProperty(Service::PresetManager::presetNameProperty, "", nullptr);
    valueTreeState.state.setProperty("version", "1.2.0", nullptr);
    valueTreeState.state.setProperty("presetPath", "", nullptr);
    presetManager = std::make_unique<Service::PresetManager>(valueTreeState);
    if (models.empty()) {
        models.resize(numModelFiles);
        unsigned long i = 0;
        for (double gain = 1.0; gain <= 10.0; gain += 0.5) {
            loadModel(1, gain, i);
            i++;
        }
        for (double gain = 1.0; gain <= 10.0; gain += 0.5) {
            loadModel(2, gain, i);
            i++;
        }
    }
    amp1_dsp = models[0];
    old_model = models[0];
    irIn = new double*[1];
    irIn[0] = new double[Constants::BUFFERSIZE];
    ampOn = false;
    fftSize = 1024;
    acf.resize(fftSize);
    all_frequencies = generateReferenceFrequencies();
    factoryIRs.resize(Constants::NUM_IRS);
    originalFactoryIRs.resize(Constants::NUM_IRS);
    for (int n = 1; n <= Constants::NUM_IRS; n++) {
        loadIR(n);
    }
    mNoiseGateTrigger.AddListener(&mNoiseGateGain);
    userIRDropdown.setTextWhenNothingSelected("Custom IRs");
    irDropdown.setTextWhenNothingSelected("Factory IRs");
    populateIRDropdown();
    for (int i = 0; i < Constants::NUM_FACTORY_PRESETS; i++) {
        loadFactoryPresets(i);
    }
    reverbRp = 0;
    reverbWetL = new float[Constants::BUFFERSIZE];
    reverbWetR = new float[Constants::BUFFERSIZE];
    for (int i = 0; i < Constants::BUFFERSIZE; i++) {
        reverbWetL[i] = 0;
        reverbWetR[i] = 0;
    }
    eq1Parameter = valueTreeState.getRawParameterValue("eq1");
    eq2Parameter = valueTreeState.getRawParameterValue("eq2");

    // Initialize LicenseSpring
    AppConfig appConfig( "Invader", "1.2.0" );
    auto pConfiguration = appConfig.createLicenseSpringConfig();
    licenseManager = LicenseSpring::LicenseManager::create( pConfiguration );
    auto license = licenseManager->getCurrentLicense();
    if (license != nullptr) {
        try {
            license->check();
            licenseActivated.store(license != nullptr && license->isValid());
        } catch (const std::exception& ex) {
            licenseActivated.store(false);
        }
    }
    else {
        licenseActivated.store(false);
    }
    licenseVisibility.store(license == nullptr || license->isTrial());

    delayMixParam = valueTreeState.getRawParameterValue("delay mix");
    delayFeedbackParam = valueTreeState.getRawParameterValue("delay feedback");
    delayTimingParam = valueTreeState.getRawParameterValue("delay timing");

    // Initialize delay objects for each channel
    channelDelays.resize(2);
    for (int ch = 0; ch < getTotalNumInputChannels(); ch++) {
        channelDelays[ch] = std::make_unique<Delay>(48000.0);
    }
}

EqAudioProcessor::~EqAudioProcessor()
{
    delete[] irIn[0];
    delete[] irIn;
}

juce::File EqAudioProcessor::writeBinaryDataToTempFile(const void* data, int size, const juce::String& fileName)
{
    // Create a temporary file
    juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile(fileName);
    DBG("path is " << tempFile.getFullPathName());
    // Ensure the file is not already there
    if (tempFile.exists()) {
        DBG("path exists, returning");
//        tempFile.deleteFile();
        return tempFile;
    }
    else {
        DBG("path doesn't exist");
    }
    
    // Write the binary data to the temporary file
    tempFile.appendData(data, size);

    return tempFile;
}

std::tuple<std::unique_ptr<juce::XmlElement>, juce::File> EqAudioProcessor::writePresetBinaryDataToTempFile(const void* data, int size, const juce::String& fileName)
{
    // Define the file path where the preset was saved
    juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile(fileName);
    // Ensure the file is not already there
    if (tempFile.exists())
        tempFile.deleteFile();
    
    // Write the binary data to the temporary file
    tempFile.replaceWithData(data, size);
    juce::String fileContent = tempFile.loadFileAsString();
    std::unique_ptr<juce::XmlElement> xmlElement(juce::XmlDocument::parse(fileContent));
    return std::make_tuple(std::move(xmlElement), tempFile);
}

void EqAudioProcessor::loadFactoryPresets(int i) {
     juce::String name = Constants::factoryPresets[i];
     const void* data;
     int size;
     if (i == 0) {
         data = BinaryData::_1__The_Rocker_preset;
         size = BinaryData::_1__The_Rocker_presetSize;
     }
     else if (i == 1) {
         data = BinaryData::_2__Capt__Crunch_preset;
         size = BinaryData::_2__Capt__Crunch_presetSize;
     }
     else if (i == 2) {
         data = BinaryData::_3__EhISee_preset;
         size = BinaryData::_3__EhISee_presetSize;
     }
     else if (i == 3) {
         data = BinaryData::_4__Soaring_Lead_preset;
         size = BinaryData::_4__Soaring_Lead_presetSize;
     }
     else if (i == 4) {
         data = BinaryData::_5__Cleaning_Up_preset;
         size = BinaryData::_5__Cleaning_Up_presetSize;
     }
     else if (i == 5) {
         data = BinaryData::_6__Sweet_Tea_Blues_preset;
         size = BinaryData::_6__Sweet_Tea_Blues_presetSize;
     }
     else if (i == 6) {
         data = BinaryData::_7__Rokk_preset;
         size = BinaryData::_7__Rokk_presetSize;
     }
     else if (i == 7) {
         data = BinaryData::_8__Crisp__Clear_preset;
         size = BinaryData::_8__Crisp__Clear_presetSize;
     }
     else if (i == 8) {
         data = BinaryData::_9__Modern_Singles_preset;
         size = BinaryData::_9__Modern_Singles_presetSize;
     }
     else if (i == 9) {
         data = BinaryData::_10__Modern_Rhythms_preset;
         size = BinaryData::_10__Modern_Rhythms_presetSize;
     }
     else if (i == 10) {
         data = BinaryData::_11__Anger_Management_preset;
         size = BinaryData::_11__Anger_Management_presetSize;
     }
     else if (i == 11) {
         data = BinaryData::_12__Psychedlica_preset;
         size = BinaryData::_12__Psychedlica_presetSize;
     }
     else if (i == 12) {
         data = BinaryData::_13__Flying_Solo_preset;
         size = BinaryData::_13__Flying_Solo_presetSize;
     }
     else if (i == 13) {
         data = BinaryData::_14__Texas_Blooze_preset;
         size = BinaryData::_14__Texas_Blooze_presetSize;
     }
     else if (i == 14) {
         data = BinaryData::_15__Vibin_preset;
         size = BinaryData::_15__Vibin_presetSize;
     }
     else if (i == 15) {
         data = BinaryData::_16__Dreamscape_preset;
         size = BinaryData::_16__Dreamscape_presetSize;
     }
     else if (i == 16) {
         data = BinaryData::_17__Thrasher_preset;
         size = BinaryData::_17__Thrasher_presetSize;
     }
     else if (i == 17) {
         data = BinaryData::_18__Down_Under_preset;
         size = BinaryData::_18__Down_Under_presetSize;
     }
     name += ".preset";
     if (data != nullptr && size > 0)
     {
         auto [xml, tempFile] = writePresetBinaryDataToTempFile(data, size, name);
         juce::String tempFilePath = tempFile.getFullPathName();
         juce::File file(tempFilePath);
         if (!file.existsAsFile())
         {
             juce::Logger::writeToLog("Error: File does not exist at " + tempFilePath);
         }
         else {
             presetManager->saveFactoryPreset(file.getFileNameWithoutExtension(), std::move(xml));
         }
     }
}

void EqAudioProcessor::loadModel(const int amp_idx, double gainLvl, unsigned long i) {
    return;
    juce::String modelName = "AMP"+juce::String(amp_idx)+"-GAIN"+juce::String(gainLvl, 1)+".wav.nam";
    const void* modelData = nullptr;
    int modelSize = 0;

    if (modelName == "AMP1-GAIN1.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN1_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN1_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN1.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN1_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN1_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN2.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN2_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN2_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN2.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN2_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN2_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN3.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN3_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN3_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN3.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN3_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN3_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN4.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN4_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN4_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN4.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN4_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN4_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN5.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN5_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN5_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN5.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN5_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN5_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN6.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN6_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN6_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN6.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN6_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN6_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN7.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN7_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN7_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN7.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN7_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN7_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN8.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN8_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN8_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN8.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN8_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN8_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN9.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN9_0_wav_nam;
        modelSize = BinaryData::AMP1GAIN9_0_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN9.5.wav.nam") {
        modelData = BinaryData::AMP1GAIN9_5_wav_nam;
        modelSize = BinaryData::AMP1GAIN9_5_wav_namSize;
    }
    else if (modelName == "AMP1-GAIN10.0.wav.nam") {
        modelData = BinaryData::AMP1GAIN10_wav_nam;
        modelSize = BinaryData::AMP1GAIN10_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN1.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN1_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN1_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN1.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN1_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN1_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN2.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN2_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN2_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN2.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN2_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN2_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN3.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN3_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN3_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN3.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN3_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN3_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN4.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN4_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN4_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN4.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN4_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN4_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN5.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN5_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN5_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN5.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN5_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN5_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN6.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN6_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN6_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN6.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN6_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN6_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN7.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN7_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN7_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN7.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN7_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN7_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN8.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN8_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN8_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN8.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN8_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN8_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN9.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN9_0_wav_nam;
        modelSize = BinaryData::AMP2GAIN9_0_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN9.5.wav.nam") {
        modelData = BinaryData::AMP2GAIN9_5_wav_nam;
        modelSize = BinaryData::AMP2GAIN9_5_wav_namSize;
    }
    else if (modelName == "AMP2-GAIN10.0.wav.nam") {
        modelData = BinaryData::AMP2GAIN10_wav_nam;
        modelSize = BinaryData::AMP2GAIN10_wav_namSize;
    }

    if (modelData != nullptr && modelSize > 0)
    {
//        juce::File tempFile = writeBinaryDataToTempFile(modelData, modelSize, modelName);
//        models[i] = nam::get_dsp(tempFile.getFullPathName().toRawUTF8());
        juce::MemoryInputStream memoryStream(modelData, modelSize, false);

        // Read and parse the JSON
        juce::var jsonParsed = juce::JSON::parse(memoryStream);
        if (jsonParsed.isObject())
        {
            auto* dynamicObject = jsonParsed.getDynamicObject();
            if (dynamicObject != nullptr)
            {
                nam::dspData conf;
                for (const auto& property : dynamicObject->getProperties())
                {
                    juce::String attributeName = property.name.toString();
                    juce::var attributeValue = property.value;
                    if (attributeName == "version") {
                        conf.version = attributeValue.toString().toStdString();
                    }
                    else if (attributeName == "architecture") {
                        conf.architecture = attributeValue.toString().toStdString();
                    }
                    else if (attributeName == "config") {
                        auto* configDynamicObject = attributeValue.getDynamicObject();
                        juce::String jsonString = juce::JSON::toString(juce::var(configDynamicObject));
                        nlohmann::json jsonObject = nlohmann::json::parse(jsonString.toStdString());
                        conf.config = jsonObject;
                    }
                    else if (attributeName == "metadata") {
                        auto* configDynamicObject = attributeValue.getDynamicObject();
                        juce::String jsonString = juce::JSON::toString(juce::var(configDynamicObject));
                        nlohmann::json jsonObject = nlohmann::json::parse(jsonString.toStdString());
                        conf.config = jsonObject;
                    }
                    else if (attributeName == "weights") {
                        if (attributeValue.isArray()) {
                            auto* array = attributeValue.getArray();
                            if (array != nullptr) {
                                for (const auto& element : *array) {
                                    if (element.isDouble()) {
                                        float value = static_cast<float>(static_cast<double>(element));
                                        conf.weights.push_back(value);
                                    }
                                }
                            }
                        }
                    }
                    else if (attributeName == "sample_rate") {
                        conf.expected_sample_rate = static_cast<double>(attributeValue);
                    }
                }
                models[i] = nam::get_dsp(conf);
            }
        }
    }
}

void EqAudioProcessor::loadIR(const int i, double sampleRate) {
    return;
    const char* irData = nullptr;
    int irSize = 0;
    juce::String irName;
    if (i == 1) {
        irData = BinaryData::Invader_1_bin;
        irSize = BinaryData::Invader_1_binSize;
        irName = "Invader 1";
    }
    else if (i == 2) {
        irData = BinaryData::Invader_2_bin;
        irSize = BinaryData::Invader_2_binSize;
        irName = "Invader 2";
    }
    else if (i == 3) {
        irData = BinaryData::Invader_3_bin;
        irSize = BinaryData::Invader_3_binSize;
        irName = "Invader 3";
    }
    else if (i == 4) {
        irData = BinaryData::Invader_4_bin;
        irSize = BinaryData::Invader_4_binSize;
        irName = "Invader 4";
    }
    else if (i == 5) {
        irData = BinaryData::Invader_5_bin;
        irSize = BinaryData::Invader_5_binSize;
        irName = "Invader 5";
    }
    else if (i == 6) {
        irData = BinaryData::Invader_6_bin;
        irSize = BinaryData::Invader_6_binSize;
        irName = "Invader 6";
    }
    else if (i == 7) {
        irData = BinaryData::Invader_7_bin;
        irSize = BinaryData::Invader_7_binSize;
        irName = "Invader 7";
    }
    else if (i == 8) {
        irData = BinaryData::Invader_8_bin;
        irSize = BinaryData::Invader_8_binSize;
        irName = "Invader 8";
    }
    else if (i == 9) {
        irData = BinaryData::Invader_9_bin;
        irSize = BinaryData::Invader_9_binSize;
        irName = "Invader 9";
    }
    else if (i == 10) {
        irData = BinaryData::Invader_10_bin;
        irSize = BinaryData::Invader_10_binSize;
        irName = "Invader 10";
    }
    else if (i == 11) {
        irData = BinaryData::Invader_11_bin;
        irSize = BinaryData::Invader_11_binSize;
        irName = "Invader 11";
    }
    else if (i == 12) {
        irData = BinaryData::Invader_12_bin;
        irSize = BinaryData::Invader_12_binSize;
        irName = "Invader 12";
    }
    else if (i == 13) {
        irData = BinaryData::Invader_13_bin;
        irSize = BinaryData::Invader_13_binSize;
        irName = "Invader 13";
    }
    else if (i == 14) {
        irData = BinaryData::Invader_14_bin;
        irSize = BinaryData::Invader_14_binSize;
        irName = "Invader 14";
    }
    else if (i == 15) {
        irData = BinaryData::Invader_15_bin;
        irSize = BinaryData::Invader_15_binSize;
        irName = "Invader 15";
    }
    else if (i == 16) {
        irData = BinaryData::Invader_16_bin;
        irSize = BinaryData::Invader_16_binSize;
        irName = "Invader 16";
    }
    else if (i == 17) {
        irData = BinaryData::Invader_17_bin;
        irSize = BinaryData::Invader_17_binSize;
        irName = "Invader 17";
    }
    else if (i == 18) {
        irData = BinaryData::Invader_18_bin;
        irSize = BinaryData::Invader_18_binSize;
        irName = "Invader 18";
    }
    if (irData != nullptr && irSize > 0) {
        dsp::ImpulseResponse::IRData irInfo;
        size_t numSamples = (irSize-sizeof(double))/sizeof(float);
        const float* audioSamples = reinterpret_cast<const float*>(irData + sizeof(double));
        irInfo.mRawAudio.resize(0);
        for (int n = 0; n < numSamples; n++) {
            irInfo.mRawAudio.push_back(audioSamples[n]);
        }
        irInfo.mRawAudioSampleRate = 48000.0;
        factoryIRs[i-1] = std::make_shared<dsp::ImpulseResponse>(irInfo, sampleRate);
        originalFactoryIRs[i-1] = std::make_shared<dsp::ImpulseResponse>(irInfo, sampleRate);
    }
}

juce::StringArray EqAudioProcessor::loadUserIRsFromDirectory(const juce::String& customIRPath)
{
    juce::StringArray customIRs;
    
    juce::File file(customIRPath);
    if (!file.existsAsFile()) {
        mStagedIR = nullptr;
        mIR = nullptr;
        return customIRs;
    }
    
    juce::File directory = file.getParentDirectory();
    if (!directory.exists()) {
        return customIRs;
    }
    
    juce::WildcardFileFilter fileFilter("*.wav", "", "WAV files");
    juce::Array<juce::File> wavFiles;
    directory.findChildFiles(wavFiles, juce::File::findFiles, false, "*.wav");
    
    if (wavFiles.isEmpty()) {
        return customIRs;
    }
    
    std::sort(wavFiles.begin(), wavFiles.end(), [](const juce::File& a, const juce::File& b) {
        return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
    });
    
    userIRs.resize(wavFiles.size());
    originalUserIRs.resize(wavFiles.size());
    userIRDropdown.clear(juce::dontSendNotification);
    
    for (int i = 1; i <= wavFiles.size(); i++) {
        juce::File irFile = wavFiles[i-1];
        juce::String path = irFile.getFullPathName();
        juce::String irName = irFile.getFileNameWithoutExtension();
        userIRDropdown.addItem(irName, i);
        customIRs.add(irName);
        
        // Read sample rate from WAV file
        std::unique_ptr<juce::AudioFormatReader> reader;
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        reader.reset(formatManager.createReaderFor(irFile));
        
        double wavSampleRate = 48000.0; // fallback default
        if (reader != nullptr) {
            wavSampleRate = reader->sampleRate;
        }
        
        userIRs[i-1] = std::make_shared<dsp::ImpulseResponse>(path.toRawUTF8(), wavSampleRate);
        originalUserIRs[i-1] = std::make_shared<dsp::ImpulseResponse>(path.toRawUTF8(), wavSampleRate);
    }
    
    userIRDropdown.addItem("Off", wavFiles.size()+1);
    customIRs.add("Off");
    
    return customIRs;
}

//==============================================================================
const juce::String EqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EqAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EqAudioProcessor::getProgramName (int index)
{
    return {};
}

void EqAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    PN150.setSr(sampleRate);
    PN800.setSr(sampleRate);
    PN4k.setSr(sampleRate);
    HS5k.setSr(sampleRate);
    float tSmooth = 0.2;
    inputGain.reset(sampleRate, tSmooth);
    outputGain.reset(sampleRate, tSmooth);
    hallWet.reset(sampleRate, tSmooth);
    eq1Gain.reset(sampleRate, tSmooth);
    eq2Gain.reset(sampleRate, tSmooth);
    in_buf.resize(fftSize);
    orderedInBuf.resize(fftSize);
    inPtr = 0;
    smpCnt = 0;
    pitch = 0;
    sr = sampleRate;
    interpSize = (unsigned int)(0.2*sampleRate);
    presetInterpSize = (unsigned int)(0.5*sampleRate);
    interpBuf.resize(interpSize);
    K = tan(M_PI*3000.0/sampleRate);
    b0 = K/(K+1);
    b1 = K/(K+1);
    a1 = (K-1)/(K+1);
    y1 = 0;
    x1 = 0;
    lpfMix = 1.f;
    rmsIn.reset(sampleRate, 0.5);
    rmsLeftOut.reset(sampleRate, 0.5);
    rmsLeftOut.setCurrentAndTargetValue(-100.f);
    rmsIn.setCurrentAndTargetValue(-100.f);
    reverbWp = (int)(sampleRate*0.035);
    projectSr = sampleRate;
    mResampler1.Reset(projectSr, Constants::BUFFERSIZE);
    mResampler2.Reset(projectSr, Constants::BUFFERSIZE);
    irResampler.Reset(projectSr, Constants::BUFFERSIZE);
    resampleFactoryIRs(projectSr);
    resampleUserIRs(projectSr);

    // Reset delay for new sample rate
    for (int ch = 0; ch < getTotalNumInputChannels(); ch++) {
        channelDelays[ch]->reset(sampleRate);
    }
}

void EqAudioProcessor::resampleFactoryIRs(double targetSr)
{
    return;
    for (int i = 0; i < Constants::NUM_IRS; i++) {
        const auto irData = originalFactoryIRs[i]->GetData();
        factoryIRs[i] = std::make_unique<dsp::ImpulseResponse>(irData, targetSr);
    }
}

void EqAudioProcessor::resampleUserIRs(double targetSr)
{
    return;
    for (int i = 0; i < userIRs.size(); i++) {
        const auto irData = originalUserIRs[i]->GetData();
        userIRs[i] = std::make_unique<dsp::ImpulseResponse>(irData, targetSr);
    }
}

void EqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    return;
    if (licenseActivated.load() && !licenseVisibility.load()) {
        float* chL = buffer.getWritePointer(0);
        float* chR = nullptr;
        if (totalNumInputChannels > 1) {
            chR = buffer.getWritePointer(1);
        }
        float input_gain = valueTreeState.getParameterAsValue("input gain").getValue();
        inputGain.setTargetValue(pow(10, input_gain/10));
        for (int ch = 0; ch < totalNumInputChannels; ch++) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); s++) {
                channelData[s] *= inputGain.getNextValue();
            }
        }
        double sum = 0;
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            dataIn[i] = totalNumInputChannels > 1 ? (NAM_SAMPLE)(0.5f*(chL[i]+chR[i])) : (NAM_SAMPLE)(chL[i]);
            auto value = dataIn[i];
            sum += value*value;
        }
        auto rmsIn_val = Decibels::gainToDecibels(sqrt((float)sum/(float)buffer.getNumSamples()));
        rmsIn.skip(buffer.getNumSamples());
        if (rmsIn_val < rmsIn.getCurrentValue()) {
            rmsIn.setTargetValue(rmsIn_val);
        }
        else {
            rmsIn.setCurrentAndTargetValue(rmsIn_val);
        }
        NAM_SAMPLE* dataInPtr = dataIn.data();
        NAM_SAMPLE** triggerOut = &dataInPtr;
        NAM_SAMPLE* dataOutPtr = dataOut.data();
        NAM_SAMPLE* cfPtr = crossfadeBuffer.data();
        const double threshold = valueTreeState.getParameterAsValue("noise gate").getValue();
        if (threshold >= -99.9)
        {
            const double time = 0.01;
            const double ratio = 0.1; // Quadratic...
            const double openTime = 0.005;
            const double holdTime = 0.0;
            const double closeTime = 0.01;
            const dsp::noise_gate::TriggerParams triggerParams(time, threshold, ratio, openTime, holdTime, closeTime);
            mNoiseGateTrigger.SetParams(triggerParams);
            mNoiseGateTrigger.SetSampleRate(projectSr);
            triggerOut = mNoiseGateTrigger.Process(&dataInPtr, 1, buffer.getNumSamples());
        }
        // this is like _applyDSPStaging()
        if (amp1_dsp != nullptr) {
            amp1_model = amp1_dsp;
            amp1_dsp = nullptr;
        }
        // this is the actual processing
        if (amp1_model != nullptr) {
            if (projectSr != modelSr) {
                mResampler1.ProcessBlock(&dataInPtr, &dataOutPtr, buffer.getNumSamples(), setResamplingModelProcess(amp1_model));
            }
            else {
                amp1_model->process(*triggerOut, dataOutPtr, buffer.getNumSamples());
                amp1_model->finalize_(buffer.getNumSamples());
            }
            bool needSmoothing = valueTreeState.getParameterAsValue("amp smooth").getValue();
            if (needSmoothing) {
                if (projectSr != modelSr) {
                    mResampler2.ProcessBlock(&dataInPtr, &cfPtr, buffer.getNumSamples(), setResamplingModelProcess(old_model));
                }
                else {
                    old_model->process(dataInPtr, cfPtr, buffer.getNumSamples());
                    old_model->finalize_(buffer.getNumSamples());
                }
                for (int s = 0; s < buffer.getNumSamples(); s++) {
                    mix = (float)interpSmplCnt/(float)interpSize;
                    float x = mix*dataOutPtr[s]+(1.f-mix)*cfPtr[s];
                    float y = b0*x+b1*x1-a1*y1;
                    x1 = x;
                    y1 = y;
                    dataOutPtr[s] = (1.f-mix)*y+mix*x;
                    interpSmplCnt++;
                    if (interpSmplCnt >= interpSize) {
                        mix = 0.f;
                        x1 = 0;
                        y1 = 0;
                        valueTreeState.getParameterAsValue("amp smooth").setValue(false);
                        old_model = amp1_model;
                        interpSmplCnt = 0;
                        break;
                    }
                }
            }
        }
        else {
            for (int s = 0; s < buffer.getNumSamples(); s++) {
                dataOutPtr[s] = triggerOut[0][s];
            }
        }
        if (threshold >= -99.9) {
            NAM_SAMPLE** gateGainOutput = mNoiseGateGain.Process(&dataOutPtr, 1, buffer.getNumSamples());
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                chL[i] = gateGainOutput[0][i];
                if (totalNumInputChannels > 1) {
                    chR[i] = gateGainOutput[0][i];
                }
            }
        }
        else {
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                chL[i] = dataOutPtr[i];
                if (totalNumInputChannels > 1) {
                    chR[i] = dataOutPtr[i];
                }
            }
        }
        // Check if mStagedIR is not null, and log that it will be processed
        if (mStagedIR != nullptr) {
            mIR = mStagedIR;
            mStagedIR = nullptr;
        }
        if (mIR != nullptr && irEnabled.load()) {
            // Log the size of the buffer and IR data to check for discrepancies
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                irIn[0][i] = (double)(chL[i]);
            }
            double** ir_out = nullptr;
            ir_out = mIR->Process(irIn, 1, buffer.getNumSamples());
            for (int ch = 0; ch < totalNumInputChannels; ch++) {
                auto *channelData = buffer.getWritePointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); i++) {
                    channelData[i] = (float)ir_out[0][i];
                }
            }
        }
        eq1Gain.setTargetValue((*eq1Parameter).load());
        eq2Gain.setTargetValue((*eq2Parameter).load());
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int i = 0; i < buffer.getNumSamples(); i++) {
                float x = channelData[i];
                float eq1Val = eq1Gain.getNextValue();  // -6 to +6 dB
                float eq2Val = eq2Gain.getNextValue();    // -6 to +6 dB

                // Apply eq1 EQ
                PN150.setValues(eq1Val, "g");
                float y150 = PN150.applyPN(x, channel);
                PN800.setValues(2.0f * eq1Val, "g");
                float y800 = PN800.applyPN(y150, channel);

                // Apply eq2 EQ
                PN4k.setValues(2.0f * eq2Val, "g");
                float y4k = PN4k.applyPN(y800, channel);
                HS5k.setValues(2.0f + eq2Val, "g");
                float y5k = HS5k.applyHS(y4k, channel);

                // Apply global EQ
                float yGlobalEQ = GlobalEQ.applyPN(y5k, channel);
                channelData[i] = yGlobalEQ;
            }
        }
        float reverbMix = valueTreeState.getParameterAsValue("reverb").getValue();
        Hall->wet = 3.0*reverbMix/1.6666666666667;
        if (Hall->wet > 0.0) {
            if (totalNumInputChannels > 1) {
                applyReverb(Hall, chL, chR, reverbWetL, reverbWetR, &reverbWp, buffer.getNumSamples(), Constants::BUFFERSIZE, 2);
                for (int i = 0; i < buffer.getNumSamples(); i++) {
                    chL[i] += reverbWetL[reverbRp];
                    chR[i] += reverbWetR[reverbRp];
                    reverbRp++;
                    if (reverbRp >= Constants::BUFFERSIZE) {
                        reverbRp = 0;
                    }
                }
            }
            else {
                applyReverb(Hall, chL, chL, reverbWetL, reverbWetR, &reverbWp, buffer.getNumSamples(), Constants::BUFFERSIZE, 1);
                for (int i = 0; i < buffer.getNumSamples(); i++) {
                    chL[i] += reverbWetL[reverbRp];
                    reverbRp++;
                    if (reverbRp >= Constants::BUFFERSIZE) {
                        reverbRp = 0;
                    }
                }
            }
        }

        // DELAY (after reverb, before output gain)
        if (auto* playHead = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo positionInfo;
            if (playHead->getCurrentPosition(positionInfo) && positionInfo.bpm > 0)
            {
                double bpm = juce::JUCEApplicationBase::isStandaloneApp() ? 80.0 : positionInfo.bpm;
                double beatsPerSec = bpm/60;
                double samplesPerBeat = sr*(60.0/bpm);
                
                // Calculate delay samples based on timing parameter
                // t = triplet (2/3), d = dotted (1.5x)
                float delayMix = delayMixParam->load();
                if (delayMix <= 0.2f) {
                    beatDivision = 0.125;
                }
                else if (delayMix <= 0.4f) {
                    beatDivision = 0.25;
                }
                else if (delayMix <= 0.6f) {
                    beatDivision = 0.5;
                }
                else if (delayMix <= 0.8f) {
                    beatDivision = 0.75;
                }
                else {
                    beatDivision = 1.5;
                }
                
                if (delayMix <= 0.2f) {
                    actualDelayMix = delayMix*0.9;
                }
                else if (delayMix <= 0.5f) {
                    actualDelayMix = 0.4*delayMix+0.1;
                }
                else {
                    actualDelayMix = 0.2*delayMix+0.2;
                }
                
                if (delayMix <= 0.25f) {
                    channelDelays[0]->FB = 0.8*delayMix;
                }
                else if (delayMix <= 0.6f) {
                    channelDelays[0]->FB = (0.2/0.35)*delayMix+0.05714285714;
                }
                else if (delayMix <= 0.8f) {
                    channelDelays[0]->FB = delayMix-0.2;
                }
                else {
                    channelDelays[0]->FB = 0.75*delayMix;
                }

                int delaySamples = (int)(samplesPerBeat * beatDivision);

                auto *data = buffer.getWritePointer(0);
                channelDelays[0]->process(data, buffer.getNumSamples(), delaySamples, actualDelayMix);
                data = buffer.getWritePointer(1);
                channelDelays[1]->FB = channelDelays[0]->FB;
                channelDelays[1]->process(data, buffer.getNumSamples(), delaySamples, actualDelayMix);
            }
        }

        float output_gain = valueTreeState.getParameterAsValue("output gain").getValue();
        outputGain.setTargetValue(pow(10, output_gain/20));
        for (int ch = 0; ch < totalNumInputChannels; ch++) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); s++) {
                channelData[s] *= outputGain.getNextValue();
            }
        }
        
        if (presetSmoothing.load()) {
            for (int ch = 0; ch < totalNumInputChannels; ch++) {
                auto* channelData = buffer.getWritePointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); s++) {
                    channelData[s] *= (float)presetInterpSmplCnt[ch]/(float)presetInterpSize;
                    presetInterpSmplCnt[ch]++;
                    if (presetInterpSmplCnt[ch] >= presetInterpSize) {
                        presetInterpSmplCnt[ch] = 0;
                        presetSmoothing.store(false);
                        break;
                    }
                }
            }
        }
        
        auto rmsLeftOut_val = Decibels::gainToDecibels(buffer.getRMSLevel(0, 0, buffer.getNumSamples()));
        rmsLeftOut.skip(buffer.getNumSamples());
        if (rmsLeftOut_val < rmsLeftOut.getCurrentValue()) {
            rmsLeftOut.setTargetValue(rmsLeftOut_val);
        }
        else {
            rmsLeftOut.setCurrentAndTargetValue(rmsLeftOut_val);
        }
    }
    else {
        // Clear buffer when license is not activated or license page is visible
        for (int ch = 0; ch < totalNumInputChannels; ch++) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); s++) {
                channelData[s] = 0;
            }
        }
    }
}

float EqAudioProcessor::getInRMS() {
    return rmsIn.getCurrentValue();
}

float EqAudioProcessor::getOutRMS(int ch) {
    return rmsLeftOut.getCurrentValue();
    return 0.f;
}

//==============================================================================
bool EqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EqAudioProcessor::createEditor()
{
    if(wrapperType == wrapperType_Standalone)
    {
        if(TopLevelWindow::getNumTopLevelWindows() == 1)
        {
            TopLevelWindow* w = TopLevelWindow::getTopLevelWindow(0);
            w->setUsingNativeTitleBar(true);
        }
    }
    return new EqAudioProcessorEditor (*this, valueTreeState);
}

//==============================================================================
void EqAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
//    juce::ValueTree state = valueTreeState.copyState();
    juce::ValueTree state = valueTreeState.state;
    DBG("Saved state before:\n");
    for (int i = 0; i < state.getNumProperties(); ++i)
    {
        auto propertyName = state.getPropertyName(i).toString();
        auto propertyValue = state[propertyName].toString();
        DBG(propertyName + ": " + propertyValue << "\n");
    }
    bool irDropdownState = lastTouchedDropdown == &irDropdown;
    state.setProperty("lastTouchedDropdown", irDropdownState, nullptr);
    state.setProperty("presetPath", presetPath, nullptr);
    // NOTE: currentMainKnobID is no longer saved - it's derived from lastBottomButton + boolean parameters
    DBG("SAVING lastBottomButton: " << lastBottomButton);
    DBG("SAVING is eq 1: " << valueTreeState.getRawParameterValue("is eq 1")->load());
    DBG("SAVING is fx 1: " << valueTreeState.getRawParameterValue("is fx 1")->load());
    state.setProperty("irVisibility", irVisibility, nullptr);
    state.setProperty("lastPresetButton", lastPresetButton, nullptr);
    state.setProperty("lastBottomButton", lastBottomButton, nullptr);
    state.setProperty("presetVisibility", presetVisibility, nullptr);
    state.setProperty("p1n", p1n, nullptr);
    state.setProperty("p2n", p2n, nullptr);
    state.setProperty("p3n", p3n, nullptr);
    state.setProperty("p4n", p4n, nullptr);
    state.setProperty("p5n", p5n, nullptr);
    state.setProperty("size", sizePortion, nullptr);
    DBG("Saved state after:\n");
    for (int i = 0; i < state.getNumProperties(); ++i)
    {
        auto propertyName = state.getPropertyName(i).toString();
        auto propertyValue = state[propertyName].toString();
        DBG(propertyName + ": " + propertyValue << "\n");
    }
    const auto xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    hasLoadedState = true;  // Mark that we loaded state from settings file
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName (valueTreeState.state.getType())) {
            juce::ValueTree state = juce::ValueTree::fromXml (*xmlState);
            bool isStandalone = juce::JUCEApplicationBase::isStandaloneApp();
            DBG("from XML: \n");
            for (int i = 0; i < state.getNumProperties(); ++i)
            {
                auto propertyName = state.getPropertyName(i).toString();
                auto propertyValue = state[propertyName].toString();
                DBG(propertyName + ": " + propertyValue << "\n");
            }
            valueTreeState.replaceState (state);
            presetPath = state.getProperty("presetPath", juce::String());
            if (!state.hasProperty("lastTouchedDropdown")) {
                lastTouchedDropdown = &irDropdown;
            }
            else {
                bool irDropdownState = state.getProperty("lastTouchedDropdown");
                lastTouchedDropdown = irDropdownState ? &irDropdown : &userIRDropdown;
            }
            if (!state.hasProperty("lastPresetButton")) {
                lastPresetButton = 0;
            }
            else {
                lastPresetButton = state.getProperty("lastPresetButton");
            }
            if (!state.hasProperty("lastBottomButton")) {
                lastBottomButton = 0;
            }
            else {
                lastBottomButton = state.getProperty("lastBottomButton");
            }
            if (!state.hasProperty("lastPage1Button")) {
                lastPage1Button = 0;
            }
            else {
                lastPage1Button = state.getProperty("lastPage1Button");
            }
            if (!state.hasProperty("lastPage2Button")) {
                lastPage2Button = 6;
            }
            else {
                lastPage2Button = state.getProperty("lastPage2Button");
            }
            if (!state.hasProperty("lastDelayButton")) {
                lastDelayButton = 0;
            }
            else {
                lastDelayButton = state.getProperty("lastDelayButton");
            }
            if(!state.hasProperty("presetVisibility")) {
                presetVisibility = false;
            }
            else {
                presetVisibility = state.getProperty("presetVisibility");
            }
            // NOTE: currentMainKnobID is no longer restored - it's derived in ButtonsAndKnobs::restoreButtonState()
            DBG("RESTORED lastBottomButton: " << lastBottomButton);
            DBG("RESTORED is eq 1: " << valueTreeState.getRawParameterValue("is eq 1")->load());
            DBG("RESTORED is fx 1: " << valueTreeState.getRawParameterValue("is fx 1")->load());
            if (state.getProperty("p1n") == "The Rocker") {
                state.setProperty("p1n", p1n, nullptr);
            }
            if (state.getProperty("p2n") == "Capt. Crunch") {
                state.setProperty("p2n", p2n, nullptr);
            }
            if (state.getProperty("p3n") == "Eh-I-See!") {
                state.setProperty("p3n", p3n, nullptr);
            }
            if (state.getProperty("p4n") == "Soaring Lead") {
                state.setProperty("p4n", p4n, nullptr);
            }
            if (state.getProperty("p5n") == "Cleaning Up") {
                state.setProperty("p5n", p5n, nullptr);
            }
            if (state.hasProperty("p1n")) {
                p1n = state.getProperty("p1n");
            }
            if (state.hasProperty("p2n")) {
                p2n = state.getProperty("p2n");
            }
            if (state.hasProperty("p3n")) {
                p3n = state.getProperty("p3n");
            }
            if (state.hasProperty("p4n")) {
                p4n = state.getProperty("p4n");
            }
            if (state.hasProperty("p5n")) {
                p5n = state.getProperty("p5n");
            }
            if (!state.hasProperty("size")) {
                state.setProperty("size", 0.75, nullptr);
            }
            else {
                sizePortion = state.getProperty("size");
            }
            setAmp();
            float eq1 = valueTreeState.getParameterAsValue("eq1").getValue();
            float eq2 = valueTreeState.getParameterAsValue("eq2").getValue();
            juce::String dspPath (valueTreeState.state.getProperty("customIR"));
            juce::StringArray customIRs = loadUserIRsFromDirectory(dspPath);
            resampleUserIRs(projectSr);
            resampleFactoryIRs(projectSr);
            updateAllIRs(customIRs);
            int lastIR = valueTreeState.state.getProperty("lastIR");
            if (lastIR > irDropdown.getNumItems()) {
                userIRDropdown.setSelectedId(lastIR-irDropdown.getNumItems());
                setCustomIR(lastIR-irDropdown.getNumItems());
            }
            else {
                irDropdown.setSelectedId(lastIR);
                getFactoryIR(lastIR);
            }
        }
    }
    else {
        DBG("xml state is null\n");
    }
    stateInformationSet.store(true);
}

void EqAudioProcessor::setButtonState(int lastBottomButton, int lastPresetButton) {
    if (auto* editor = dynamic_cast<EqAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->setButtonState(lastBottomButton, lastPresetButton);
    }
}

void EqAudioProcessor::setAmp() {
    int start_idx = std::nan("-1");
    bool ampState = valueTreeState.getParameterAsValue("is amp 1").getValue();
    if (!ampState) {
        start_idx = models.size()/2;
    }
    else {
        start_idx = 0;
    }
    float gainLvl = valueTreeState.getParameterAsValue("amp gain").getValue();
    float intpart;
    float frac = std::modf(gainLvl, &intpart);
    int offset = (unsigned long)((gainLvl-1.0)*2);
    if (frac == 0.f || frac == 0.5f) {
        enableSmoothing();
    }
    model_id = start_idx+offset;
//    DBG("model_id: " << model_id << ", gain level: " << gainLvl << ", offset: " << offset);
    setModel();
}

void EqAudioProcessor::setMainKnobID() {
    if (auto* editor = dynamic_cast<EqAudioProcessorEditor*>(getActiveEditor()))
    {
        currentMainKnobID = valueTreeState.state.getProperty("currentMainKnobID", juce::String());
        editor->switchAttachmentTo();
    }
}

void EqAudioProcessor::restoreEditorButtonState() {
    if (auto* editor = dynamic_cast<EqAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->cc.restoreButtonState();
    }
}

void EqAudioProcessor::setMainKnobVal(double val) {
    if (auto* editor = dynamic_cast<EqAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->setMainKnobVal(val);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EqAudioProcessor();
}

void EqAudioProcessor::populateIRDropdown() {
    int numIRs = Constants::NUM_IRS;
    for (int i = 1; i <= numIRs; i++) {
        juce::String irName = "Invader "+juce::String(i);
        irDropdown.addItem(irName, i);
        facIRs.add(irName);
    }
    irDropdown.addItem("Off", numIRs+1);
    facIRs.add("Off");
}
