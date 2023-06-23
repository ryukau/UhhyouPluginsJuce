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

### BarBox
Barbox takes keyboard focus when clicked, or mouse wheel is rotated on it.

List of keyboard shortcuts:

- A: 'A'lternate between positive and negative sign.
- D: Reset to default.
- Shift + D: Toggle min/default/max.
- E: Emphasize low.
- Shift + E: Emphasize high.
- F: Apply low-pass filter.
- Shift + F: Apply high-pass filter.
- I: Full invert.
- Shift + I: Invert depending on sign.
- L: Lock.
- Shift + L: Lock all.
- N: Normalize depending on sign.
- Shift + N: Full normalize.
- P: Permute.
- R: Randomize.
- Shift + R: Sparse randomize.
- S: Sort descending order.
- Shift + S: Sort ascending order.
- T: Subtle randomize, or random walk.
- Shift + T: Subtle randomize. Moving towards 0.
- Z: Undo. This is only effective for a BarBox.
- Shift + Z: Redo. This is only effective for a BarBox.
- , (Comma): Rotate backward.
- . (Period): Rotate forward.
- 1-4: Decrease 1-4n.
- 5-9: Decimate and hold 2-6 samples.
)";

} // namespace Uhhyou
