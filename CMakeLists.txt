cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary.")
endif()

project(UhhyouPlugins VERSION 0.3.0)

add_subdirectory(lib)
add_subdirectory(plugin)
add_subdirectory(experimental)
