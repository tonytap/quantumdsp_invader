cmake_minimum_required(VERSION 3.22)

project(Invader VERSION 1.2.0)

juce_set_aax_sdk_path("$ENV{HOME}/Documents/aax-sdk-2-8-0")

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Quantum DSP"
    FORMATS AU VST3 AAX Standalone
    PRODUCT_NAME "Invader"
    MICROPHONE_PERMISSION_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
    ICON_BIG "${CMAKE_SOURCE_DIR}/plugin/resources/InvaderIcon.icns"
)

file(GLOB_RECURSE NAM_SOURCES NeuralAmpModelerCore/NAM/*.cpp)
file(GLOB_RECURSE DSP_SOURCES source/*.cpp)
file(GLOB_RECURSE PRESET_SOURCES include/Service/*.cpp)
file(GLOB_RECURSE IR_SOURCES dsp/*.cpp)
file(GLOB_RECURSE AMP1_FILES resources/amp1/*.nam)
file(GLOB_RECURSE BOOST_FILES resources/boost/*.nam)
file(GLOB_RECURSE IR_FILES resources/irs/*)
file(GLOB_RECURSE PRESET_FILES resources/presets/*.preset)
file(GLOB_RECURSE IMAGE_FILES resources/images/*.png)
file(GLOB_RECURSE HEADER_FILES "include/*.h")

juce_add_binary_data(Models SOURCES ${AMP1_FILES} ${BOOST_FILES} ${IR_FILES} ${PRESET_FILES} ${IMAGE_FILES})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${DSP_SOURCES}
        ${NAM_SOURCES}
        ${IR_SOURCES}
        ${PRESET_SOURCES}
        ${HEADER_FILES}
)

set(LICENSESPRING_DEBUG_DIR "${CMAKE_SOURCE_DIR}/libs/LicenseSpring/bin/static/Debug")
set(LICENSESPRING_RELEASE_DIR "${CMAKE_SOURCE_DIR}/libs/LicenseSpring/bin/static/Release")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        ${JUCE_DIR}/modules
        NeuralAmpModelerCore/Dependencies/eigen
        NeuralAmpModelerCore/Dependencies/nlohmann
        dsp
        ${LICENSESPRING_DIR}/include
)

set(LICENSESPRING_LIBRARIES
    libcrypto.a
    libcurl.a
    libLicenseSpring.a
    libssl.a
)

# Link all libraries dynamically based on build configuration (Debug/Release)
foreach(LIBRARY ${LICENSESPRING_LIBRARIES})
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            $<$<CONFIG:Debug>:${LICENSESPRING_DEBUG_DIR}/${LIBRARY}>
            $<$<CONFIG:Release>:${LICENSESPRING_RELEASE_DIR}/${LIBRARY}>
    )
endforeach()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        Models
        "-framework SystemConfiguration"
        "-framework CoreFoundation"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Enable_IAA=1
        JucePlugin_StandaloneEnableAudioInput=1
    # PRIVATE
    #     JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)
add_subdirectory(NeuralAmpModelerCore)