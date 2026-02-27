cmake_minimum_required(VERSION 3.22)

juce_add_plugin(ClickyTransient
  PRODUCT_NAME "ClickyTransient"
  VERSION "0.0.1"
  COMPANY_NAME "UhhyouPlugins"
  COMPANY_EMAIL "ryukau@gmail.com"
  COMPANY_COPYRIGHT "Copyright Takamitsu Endo"
  PLUGIN_MANUFACTURER_CODE "Uhyo"
  PLUGIN_CODE "ctr9"
  NEEDS_MIDI_INPUT TRUE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  FORMATS AU VST3)

target_sources(ClickyTransient
  PRIVATE
  dsp/dspcore.cpp
  PluginEditor.cpp
  PluginProcessor.cpp)

target_compile_definitions(ClickyTransient
  PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ClickyTransient
  PRIVATE
  UhhyouCommon
  juce::juce_audio_utils
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)
