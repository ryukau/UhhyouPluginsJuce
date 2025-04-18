cmake_minimum_required(VERSION 3.22)

juce_add_plugin(AmplitudeModulator
  PRODUCT_NAME "AmplitudeModulator"
  VERSION "0.0.1"
  COMPANY_NAME "UhhyouPlugins"
  COMPANY_EMAIL "ryukau@gmail.com"
  COMPANY_COPYRIGHT "Copyright 2024 Takamitsu Endo."
  PLUGIN_MANUFACTURER_CODE "Uhyo"
  PLUGIN_CODE "d5bd"
  NEEDS_MIDI_INPUT TRUE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  FORMATS AU VST3)

target_sources(AmplitudeModulator
  PRIVATE
  dsp/dspcore.cpp
  PluginEditor.cpp
  PluginProcessor.cpp)

target_compile_definitions(AmplitudeModulator
  PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AmplitudeModulator
  PRIVATE
  UhhyouCommon
  juce::juce_audio_utils
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)
