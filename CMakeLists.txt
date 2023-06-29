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
    $<$<CONFIG:Release>:-ffast-math -Wno-shadow>)
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

# Plugins.
include("EasyOverdrive/plugin.cmake")
include("SlopeFilter/plugin.cmake")
