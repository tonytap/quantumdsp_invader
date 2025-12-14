cmake_minimum_required(VERSION 3.22)

project(Invader VERSION 1.0.5)

juce_set_aax_sdk_path("/Users/anthony/Downloads/aax-sdk-2-8-0")

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Quantum DSP"
    FORMATS AU VST3 Standalone AAX
    PRODUCT_NAME "Invader"
    MICROPHONE_PERMISSION_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
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

juce_add_binary_data(Models SOURCES ${AMP1_FILES} ${BOOST_FILES} ${IR_FILES} ${PRESET_FILES} ${IMAGE_FILES})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${DSP_SOURCES}
        ${NAM_SOURCES}
        ${IR_SOURCES}
        ${PRESET_SOURCES}
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

# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     # Define the formats you already specified in juce_add_plugin
#     set(FORMATS "AU" "VST3" "Standalone" "AAX")
#     set(zip "zip") # Without the leading period
#     # Loop through each format and determine the appropriate file extension
#     foreach(FORMAT ${FORMATS})
#         # Derive the target name
#         set(TARGET_NAME "${PROJECT_NAME}_${FORMAT}")

#         # Determine the file extension based on the format
#         if(FORMAT STREQUAL "AU")
#             set(EXTENSION "component")
#         elseif(FORMAT STREQUAL "VST3")
#             set(EXTENSION "vst3")
#         elseif(FORMAT STREQUAL "Standalone")
#             set(EXTENSION "app")
#         elseif(FORMAT STREQUAL "AAX")
#             set(EXTENSION "aaxplugin")
#         endif()

#         # Add post-build commands for signing, notarizing, and stapling
#         add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
#             COMMAND echo "Resolved bundle directory for ${FORMAT}: $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>"
#             if (FORMAT STREQUAL "AAX")
#                 # Code sign the plugin
#                 COMMAND codesign --force --options runtime --sign "Developer ID Application: Alexandre Whitcombe (C6KTT86896)"
#                     $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>
#                 COMMENT "Code signing ${PROJECT_NAME} (${FORMAT})"
#                 VERBATIM
#             else()
#                 # Code sign the plugin
#                 COMMAND codesign --force --sign "Developer ID Application: Alexandre Whitcombe (C6KTT86896)"
#                     $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>
#                 COMMENT "Code signing ${PROJECT_NAME} (${FORMAT})"
#                 VERBATIM
#             endif()
            
#             # # Create a ZIP file for notarization
#             # COMMAND zip -r $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>/../${PROJECT_NAME}.${zip} $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>
#             # COMMENT "Zipping ${PROJECT_NAME} (${FORMAT}) for notarization"
#             # VERBATIM

#             # # Submit the ZIP file for notarization
#             # COMMAND xcrun notarytool submit 
#             #     $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>/../${PROJECT_NAME}.${zip}
#             #     --apple-id "alex@quantumdsp.com" 
#             #     --password "udoa-dgmu-mcys-sisj" 
#             #     --team-id "C6KTT86896" 
#             #     --wait
#             # COMMENT "Submitting ${PROJECT_NAME} (${FORMAT}) for notarization"
#             # VERBATIM

#             # # Staple the notarization ticket to the plugin
#             # COMMAND xcrun stapler staple $<TARGET_BUNDLE_DIR:${PROJECT_NAME}_${FORMAT}>
#             # COMMENT "Stapling notarization ticket to ${PROJECT_NAME} (${FORMAT})"
#             # VERBATIM
#         )
#     endforeach()
# endif()

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