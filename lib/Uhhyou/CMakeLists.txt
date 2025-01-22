cmake_minimum_required(VERSION 3.22)

include("cmake/additional_compiler_flag.cmake")

# Common components. Separeted to reduce build time.
add_library(UhhyouCommon OBJECT
  gui/style.cpp)

target_link_libraries(UhhyouCommon
  PRIVATE
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_options(UhhyouCommon PRIVATE -fPIC)
endif()

target_include_directories(UhhyouCommon PUBLIC "..")
target_include_directories(UhhyouCommon PRIVATE
  "."
  "../JUCE/modules")
target_compile_definitions(UhhyouCommon PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
