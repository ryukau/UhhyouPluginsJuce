// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#include <juce_core/juce_core.h>

namespace Uhhyou {

const char *informationText = JucePlugin_Name " version " JucePlugin_VersionString R"(

Click outside to close this message.

## Controls
### Knobs & Number Sliders
Right Click: Open host context menu.
Wheel Click: Rotate throught min/default/max value.
Ctrl + Left Click: Reset to default.
Ctrl + Wheel Click: Take floor of current value.
)";

} // namespace Uhhyou
