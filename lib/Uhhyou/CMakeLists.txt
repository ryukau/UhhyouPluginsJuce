cmake_minimum_required(VERSION 3.22)

include("cmake/additional_compiler_flag.cmake")

# Common components. Separeted to reduce build time.
add_library(UhhyouCommon OBJECT
  gui/style.cpp)

target_link_libraries(UhhyouCommon PRIVATE
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
  additional_compiler_flag)

set_target_properties(UhhyouCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(UhhyouCommon PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
get_target_property(JUCE_INCLUDE_DIRS juce::juce_core INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(UhhyouCommon PRIVATE "." ${JUCE_INCLUDE_DIRS})
target_include_directories(UhhyouCommon PUBLIC "..")

juce_add_binary_data(UhhyouLicenseData
  SOURCES ../README.md
  NAMESPACE UhhyouLicense)
target_link_libraries(UhhyouCommon PUBLIC UhhyouLicenseData)
