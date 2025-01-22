cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE) # Build universal binary.

project(UhhyouPlugins VERSION 0.2.0)

add_subdirectory(lib)
add_subdirectory(experimental)
