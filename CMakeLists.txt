cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE) # Build universal binary.

project(UhhyouPluginsJuce VERSION 0.0.1)

add_subdirectory(lib/JUCE)

# # Generate license text.
# # Commented out because text too long for cl.exe. :(
# add_custom_command(
# OUTPUT
# ${CMAKE_SOURCE_DIR}/common/librarylicense.hpp
# DEPENDS
# ${CMAKE_SOURCE_DIR}/lib/README.md
# COMMAND
# ${CMAKE_COMMAND} -P common/cmake/generate_cpp_text.cmake
# WORKING_DIRECTORY
# ${CMAKE_SOURCE_DIR}
# )

# Additional compiler flags.
add_library(additional_compiler_flag INTERFACE)

if((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  OR(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
)
  target_compile_options(additional_compiler_flag
    INTERFACE
    $<$<CONFIG:Debug>:>
    $<$<CONFIG:Release>:/fp:fast>)
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  OR(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  OR(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
)
  target_compile_options(additional_compiler_flag
    INTERFACE
    $<$<CONFIG:Debug>:>
    $<$<CONFIG:Release>:-ffast-math>)
endif()

# Common components. Separeted to reduce build time.
add_library(UhhyouCommon OBJECT
  common/gui/style.cpp)

target_link_libraries(UhhyouCommon
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)

target_include_directories(UhhyouCommon PRIVATE "lib/JUCE/modules")
target_compile_definitions(UhhyouCommon PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)

# EasyOverdrive
juce_add_plugin(EasyOverdrive
  PRODUCT_NAME "EasyOverdrive"
  VERSION "0.0.1"
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
