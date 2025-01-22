cmake_minimum_required(VERSION 3.22)

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
