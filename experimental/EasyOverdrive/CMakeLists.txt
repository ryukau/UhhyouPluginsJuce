cmake_minimum_required(VERSION 3.22)

juce_add_plugin(EasyOverdrive
  PRODUCT_NAME "EasyOverdrive"
  VERSION "0.0.2"
  COMPANY_NAME "UhhyouPlugins"
  COMPANY_EMAIL "ryukau@gmail.com"
  COMPANY_COPYRIGHT "Copyright 2023 Takamitsu Endo."
  PLUGIN_MANUFACTURER_CODE "Uhyo"
  PLUGIN_CODE "4r89"
  NEEDS_MIDI_INPUT TRUE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  FORMATS AU VST3)

target_sources(EasyOverdrive
  PRIVATE
  dsp/dspcore.cpp
  PluginEditor.cpp
  PluginProcessor.cpp)

target_compile_definitions(EasyOverdrive
  PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EasyOverdrive
  PRIVATE
  UhhyouCommon
  juce::juce_audio_utils
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)
