# Developing on JUCE
This text is based on JUCE 8.0.6.

## Build Time
Build time is slow. It seems that following sources are repeatedly built for each plugins.

```
juce_audio_processors.cpp
juce_audio_processors_ara.cpp
juce_audio_processors_lv2_libs.cpp
juce_gui_extra.cpp
juce_gui_basics.cpp
juce_graphics.cpp
juce_graphics_Harfbuzz.cpp
juce_events.cpp
juce_core.cpp
juce_core_CompilationTime.cpp
juce_data_structures.cpp
juce_audio_basics.cpp
juce_graphics_Sheenbidi.c
juce_audio_plugin_client_AAX.cpp
juce_audio_plugin_client_AAX_utils.cpp
juce_audio_plugin_client_ARA.cpp
juce_audio_plugin_client_LV2.cpp
juce_audio_plugin_client_Standalone.cpp
juce_audio_plugin_client_Unity.cpp
juce_audio_plugin_client_VST2.cpp
juce_audio_plugin_client_VST3.cpp
```

`juce_add_plugin()` adds these sources. To speed up build process, they might be separately built as an object library.

Considerations if trying to bypass `juce_add_plugin()`:

- `juce_add_plugin()` provides switches to toggle `#ifdef` in these sources. Instead, plugin sources should explicitly `#define` every switches.
- One has to write own cmake to bundle each plugin formats.
