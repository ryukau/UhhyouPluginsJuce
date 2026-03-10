# Developing on JUCE
This text is based on JUCE 8.0.6.

## Build System (CMake)
### Build Time
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

### Build Target
`--target <PluginName>_All` can be used to build specific plugin.

```cmake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ShockFlanger_All
```

## Accesibility
JUCE 8 specific.

### Screen Reader on Momentary Buttons
It may be called command button.

Narrator on Windows 11 does not read the state of momentary button (`juce::AccessibilityRole::button`). It stays silent on `juce::AccessibilityEvent::valueChanged`.

Narrator also does not announce the value change on toggle button. In this repository, a hack is used to circumvent the limitation. For `juce::AccessibilityRole::toggleButton`, `juce::AccessibilityEvent::titleChanged` is used to announce value change. It feels not right because it reads up the entire title.

On audio plugins, momentary button that sends gating signal may be treated as a slider that could only set the value 0 and 1. It is probably better not to implement this unless there's a user feedback.

References that might be useful later:

- [WAI-ARIA Button Pattern](https://www.w3.org/WAI/ARIA/apg/patterns/button/)
- [UIA Button Control Type](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-supportbuttoncontroltype)
