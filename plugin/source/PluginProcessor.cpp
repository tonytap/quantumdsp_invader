/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "../include/PluginProcessor.h"
#include "../include/PluginEditor.h"
#include "../include/Service/PresetManager.h"
#include <cmath>
#include <mutex>

//==============================================================================
// Define static shared resources
std::vector<std::shared_ptr<nam::DSP>> EqAudioProcessor::models;
std::vector<std::shared_ptr<dsp::ImpulseResponse>> EqAudioProcessor::factoryIRs;
std::vector<std::shared_ptr<dsp::ImpulseResponse>> EqAudioProcessor::originalFactoryIRs;
std::once_flag EqAudioProcessor::modelsInitFlag;
std::once_flag EqAudioProcessor::irsInitFlag;

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
EQ1_1(48000.0, Constants::fc_eq1_1),
EQ1_2(48000.0, Constants::fc_eq1_2),
EQ2_1(48000.0, Constants::fc_eq2_1),
EQ2_2(48000.0, Constants::fc_eq2_2),
GlobalEQ(48000.0, Constants::fc_globalEQ),
mResampler1(48000.0),
mResampler2(48000.0),
irResampler(48000.0)
#endif
{
    GlobalEQ.setValues(Constants::gain_globalEQ, "g");
    // Initialize preset button names with first 5 factory presets
    p1n = Constants::factoryPresets[0];
    p2n = Constants::factoryPresets[1];
    p3n = Constants::factoryPresets[2];
    p4n = Constants::factoryPresets[3];
    p5n = Constants::factoryPresets[4];

    valueTreeState.state.setProperty(Service::PresetManager::presetNameProperty, "", nullptr);
    valueTreeState.state.setProperty("version", Constants::versionNum, nullptr);
    valueTreeState.state.setProperty("presetPath", "", nullptr);
    presetManager = std::make_unique<Service::PresetManager>(valueTreeState);

    // Initialize shared resources only once across all instances
    DBG("=== Creating EqAudioProcessor instance ===");
    std::call_once(modelsInitFlag, &EqAudioProcessor::initializeSharedModels);
    std::call_once(irsInitFlag, &EqAudioProcessor::initializeSharedIRs);
    DBG("=== After call_once: models.size()=" << models.size() << ", factoryIRs.size()=" << factoryIRs.size() << " ===");

    amp1_dsp = models[0];
    old_model = models[0];
    irIn = new double*[1];
    irIn[0] = new double[Constants::BUFFERSIZE];
    ampOn = false;
    fftSize = 1024;
    acf.resize(fftSize);
    all_frequencies = generateReferenceFrequencies();
    mNoiseGateTrigger.AddListener(&mNoiseGateGain);
    userIRDropdown.setTextWhenNothingSelected("Custom IRs");
    irDropdown.setTextWhenNothingSelected("Factory IRs");
    populateIRDropdown();
    for (int i = 1; i <= Constants::NUM_FACTORY_PRESETS; i++) {
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
    AppConfig appConfig( Constants::productName, Constants::versionNum );
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
    restoreIRFromState();
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
     juce::String name = Constants::factoryPresets[i-1];
     int size;
     std::string resourceName = "_"+std::to_string(i)+"_preset";
     const void* data = BinaryData::getNamedResource(resourceName.c_str(), size);
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
//    return;
    // Format gain string: replace dot with underscore, or strip .0 for integer gains
    juce::String gainStr = juce::String(gainLvl, 1);
    gainStr = gainStr.replace(".", "_");

    // Construct binary resource name: AMP{amp_idx}GAIN{gain}_wav_nam
    std::string modelBinaryName = "AMP" + std::to_string(amp_idx) + "GAIN" + gainStr.toStdString() + "_wav_nam";

    const void* modelData = nullptr;
    int modelSize = 0;
    modelData = BinaryData::getNamedResource(modelBinaryName.c_str(), modelSize);

    if (modelData != nullptr && modelSize > 0)
    {
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
//    return;
    const char* irData = nullptr;
    int irSize = 0;
    juce::String irName = juce::String(Constants::productName)+" "+juce::String(i);
    std::string irBinaryName = std::string(Constants::productName)+"_"+std::to_string(i)+"_bin";
    irData = BinaryData::getNamedResource(irBinaryName.c_str(), irSize);
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

void EqAudioProcessor::initializeSharedModels()
{
    DBG("=== INITIALIZING SHARED MODELS (should only happen once) ===");
    const int numModelFiles = 38;
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
    DBG("=== SHARED MODELS INITIALIZED: " << models.size() << " models ===");
}

void EqAudioProcessor::initializeSharedIRs()
{
    DBG("=== INITIALIZING SHARED IRs (should only happen once) ===");
    factoryIRs.resize(Constants::NUM_IRS);
    originalFactoryIRs.resize(Constants::NUM_IRS);

    for (int n = 1; n <= Constants::NUM_IRS; n++) {
        loadIR(n);
    }
    DBG("=== SHARED IRs INITIALIZED: " << factoryIRs.size() << " IRs ===");
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
    userIRPaths.resize(wavFiles.size());  // Store paths for recall
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
        userIRPaths[i-1] = path;  // Store the full path for recall
    }
    
    userIRDropdown.addItem("Off", wavFiles.size()+1);
    customIRs.add("Off");
    
    return customIRs;
}

int EqAudioProcessor::findUserIRIndexByPath(const juce::String& path)
{
    // Normalize both paths for comparison (handle different separators, etc)
    juce::File targetFile(path);
    juce::String targetPath = targetFile.getFullPathName();

    for (int i = 0; i < userIRPaths.size(); i++) {
        juce::File currentFile(userIRPaths[i]);
        if (currentFile.getFullPathName() == targetPath) {
            return i;  // Return 0-based index
        }
    }

    return -1;  // Not found
}

void EqAudioProcessor::restoreIRFromState()
{
    // Single source of truth for IR restoration logic
    // Called by both setStateInformation() and extraPresetConfig()

    auto state = valueTreeState.state;

    // ALWAYS load custom IRs if path exists (populate dropdown)
    juce::String customIRPath = state.getProperty("customIR", "");
    if (customIRPath.isNotEmpty()) {
        juce::StringArray customIRs = loadUserIRsFromDirectory(customIRPath);
        resampleUserIRs(projectSr);
        updateAllIRs(customIRs);
        DBG("Loaded custom IRs from path: " << customIRPath);
    }

    // Check lastTouchedDropdown to know which IR type is currently selected
    bool irDropdownState = state.getProperty("lastTouchedDropdown", true);
    lastTouchedDropdown = irDropdownState ? &irDropdown : &userIRDropdown;

    // Write to state tree so it gets saved
    if (!state.hasProperty("lastTouchedDropdown")) {
        state.setProperty("lastTouchedDropdown", irDropdownState, nullptr);
    }

    if (!irDropdownState) {
        // Custom IR was selected - set dropdown selection and load IR
        if (customIRPath.isNotEmpty()) {
            int foundIndex = findUserIRIndexByPath(customIRPath);
            if (foundIndex >= 0) {
                int isCustomIrOff = state.getProperty("customIrOff", false);
                if (isCustomIrOff) {
                    userIRDropdown.setSelectedId(userIRDropdown.getNumItems());
                }
                else {
                    userIRDropdown.setSelectedId(foundIndex + 1);
                }
            } else {
                DBG("Custom IR not found: " << customIRPath);
            }
        } else {
            // Fallback to index-based (old presets)
            int customOpt = state.getProperty("customIROption", 0);
            setIRName(irDropdown.getNumItems(), customOpt);
        }
    } else {
        // Factory IR was selected - set dropdown selection and load IR
        int factoryIndex = valueTreeState.getRawParameterValue("ir selection")->load();
        irDropdown.setSelectedId(factoryIndex + 1, juce::dontSendNotification);
        getFactoryIR(factoryIndex);
        DBG("Restored factory IR index: " << factoryIndex);
        userIRDropdown.setSelectedId(userIRDropdown.getNumItems(), juce::dontSendNotification);
    }
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
    EQ1_1.setSr(sampleRate);
    EQ1_2.setSr(sampleRate);
    EQ2_1.setSr(sampleRate);
    EQ2_2.setSr(sampleRate);
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
//    return;
    for (int i = 0; i < Constants::NUM_IRS; i++) {
        const auto irData = originalFactoryIRs[i]->GetData();
        factoryIRs[i] = std::make_unique<dsp::ImpulseResponse>(irData, targetSr);
    }
}

void EqAudioProcessor::resampleUserIRs(double targetSr)
{
//    return;
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
//    return;
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
                EQ1_1.setValues(Constants::eq1_1_slope*eq1Val+Constants::eq1_1_bias, "g");
                float y150 = EQ1_1.applyPN(x, channel);
                EQ1_2.setValues(Constants::eq1_2_slope*eq1Val+Constants::eq1_2_bias, "g");
                float y800 = EQ1_2.applyPN(y150, channel);

                // Apply eq2 EQ
                EQ2_1.setValues(Constants::eq2_1_slope*eq2Val+Constants::eq2_1_bias, "g");
                float y4k = EQ2_1.applyPN(y800, channel);
                EQ2_2.setValues(Constants::eq2_2_slope*eq2Val+Constants::eq2_2_bias, "g");
                float y5k = EQ2_2.applyHS(y4k, channel);

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

                // Only process right channel if stereo
                if (totalNumInputChannels > 1) {
                    data = buffer.getWritePointer(1);
                    channelDelays[1]->FB = channelDelays[0]->FB;
                    channelDelays[1]->process(data, buffer.getNumSamples(), delaySamples, actualDelayMix);
                }
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

    // Note: FL Studio may cache the result of getStateInformation() called before GUI opens
    // So we can't rely on early returns - just save what we have
    juce::ValueTree state = valueTreeState.state;
    DBG("Saved state before:\n");
    for (int i = 0; i < state.getNumProperties(); ++i)
    {
        auto propertyName = state.getPropertyName(i).toString();
        auto propertyValue = state[propertyName].toString();
        DBG(propertyName + ": " + propertyValue << "\n");
    }

    // getStateInformation() should ONLY save what's already in the state tree
    // DO NOT create properties here - they're created when buttons are clicked
    // This allows GUI constructor to detect a new instance by checking if properties exist

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
    DBG("=== setStateInformation CALLED ===");
    DBG("sizeInBytes: " + juce::String(sizeInBytes));
    DBG("isStandaloneApp: " + juce::String(juce::JUCEApplicationBase::isStandaloneApp() ? "true" : "false"));

    hasLoadedState = true;  // Mark that we loaded state from settings file
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName (valueTreeState.state.getType())) {
            juce::ValueTree state = juce::ValueTree::fromXml (*xmlState);
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
            // Write back to state tree to trigger ValueTree listeners (updates GUI)
            state.setProperty("lastPresetButton", lastPresetButton, nullptr);

            if (!state.hasProperty("lastBottomButton")) {
                lastBottomButton = 0;
            }
            else {
                lastBottomButton = state.getProperty("lastBottomButton");
            }
            // Write back to state tree to trigger ValueTree listeners (updates GUI)
            state.setProperty("lastBottomButton", lastBottomButton, nullptr);
            if(!state.hasProperty("presetVisibility")) {
                presetVisibility = false;
            }
            else {
                presetVisibility = state.getProperty("presetVisibility");
            }
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
            if (auto* editor = dynamic_cast<EqAudioProcessorEditor*>(getActiveEditor()))
            {
                editor->sizePortion = sizePortion;
                editor->setProportion();
            }
            setAmp();

            // Resample factory IRs
            resampleFactoryIRs(projectSr);

            // Restore IR state using unified helper method
            restoreIRFromState();
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
        juce::String irName = juce::String(Constants::productName)+" "+juce::String(i);
        irDropdown.addItem(irName, i);
        facIRs.add(irName);
    }
    irDropdown.addItem("Off", numIRs+1);
    facIRs.add("Off");
}
