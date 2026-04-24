cmake_minimum_required(VERSION 3.22)
project(SATURACE VERSION 1.0.0)

set(JUCE_DIR "$ENV{HOME}/JUCE" CACHE PATH "Path to JUCE root")
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(SATURACE
    VERSION                     "1.0.0"
    COMPANY_NAME                "Nowaa"
    PLUGIN_MANUFACTURER_CODE    "Nwaa"
    PLUGIN_CODE                 "Satr"
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "SATURACE"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Distortion"
)

juce_generate_juce_header(SATURACE)

target_sources(SATURACE PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

target_compile_definitions(SATURACE
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SATURACE
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
