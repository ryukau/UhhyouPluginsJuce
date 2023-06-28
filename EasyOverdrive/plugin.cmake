# Top level CMakeLists.txt includes this file.

juce_add_plugin(EasyOverdrive
  PRODUCT_NAME "EasyOverdrive"
  VERSION "0.0.2"
  COMPANY_NAME "Uhhyou Plugins"
  COMPANY_EMAIL "ryukau@gmail.com"
  COMPANY_COPYRIGHT "Copyright 2023 Takamitsu Endo."
  PLUGIN_MANUFACTURER_CODE "Uhyo"
  PLUGIN_CODE "4r89"
  NEEDS_MIDI_INPUT TRUE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  FORMATS AU VST3)

target_sources(EasyOverdrive
  PRIVATE
  common/librarylicense.hpp
  EasyOverdrive/dsp/dspcore.cpp
  EasyOverdrive/PluginEditor.cpp
  EasyOverdrive/PluginProcessor.cpp)

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
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)
