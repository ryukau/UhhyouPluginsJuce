cmake_minimum_required(VERSION 3.22)

add_library(additional_compiler_flag INTERFACE)

if((CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  if((CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
    target_compile_options(additional_compiler_flag
      INTERFACE
      $<$<CONFIG:Release>:/fp:fast>)
  elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang")) # clang-cl.exe
    target_compile_options(additional_compiler_flag
      INTERFACE
      $<$<CONFIG:Release>:
      /fp:fast
      -Wno-nan-infinity-disabled # JUCE 8
      -Wno-nontrivial-memcall # JUCE 8
      -Wno-deprecated-literal-operator # nlohmann::json
      >)
  endif()
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  OR(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  OR(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
)
  target_compile_options(additional_compiler_flag
    INTERFACE
    $<$<CONFIG:Release>:
    -ffast-math
    -Wno-nan-infinity-disabled
    -Wno-deprecated-literal-operator>)
endif()
