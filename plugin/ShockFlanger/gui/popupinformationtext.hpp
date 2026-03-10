// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include <juce_core/juce_core.h>

namespace Uhhyou {

const char* informationText = JucePlugin_Name " version " JucePlugin_VersionString R"(

To close this message, click the margin outside of tab view.

--------
Controls
--------

NOTE: If DAW shortcuts fail, disable "Keyboard Navigation" in GUI Settings.

-- Keyboard Navigation --
Navigate        : Tab | Shift + Tab
Interact        : Space | Enter
Close | Back    : Esc
Focus Memory    : Auto-restores last control after Alt + Tab

-- Knob --
Adjust          : Vertical Drag | Scroll | Arrows
Fine-Tune       : Shift + Adjust
Snap Next       : Mid-Click
Snap Cycle      : Home | End | @MOD@ + Arrow
Snap Integer    : Shift + Mid-Click
Exact Value     : Dbl-Click | Enter | Space
Reset           : @MOD@ + Click | Backspace | Del
Host Menu       : Right-Click | Shift + F10

-- XY Pad --
Adjust Both     : Drag | Arrows (X = Left/Right, Y = Up/Down)
Adjust X | Y    : Scroll (X) | Shift + Scroll (Y)
Move X Only     : Mid-Click + Drag
Move Y Only     : Shift + Mid-Click + Drag
Toggle Grid     : Click square toggle
Snap Cycle X    : PgUp | PgDn
Snap Cycle Y    : Home | End
Reset Both      : @MOD@ + Click | Backspace | Del
Host Menu X     : Right-Click L-half | Shift + F10
Host Menu Y     : Right-Click R-half | @MOD@ + Shift + F10

-- Combo Box --
Open Menu       : Click | Enter | Space
Quick Cycle     : Scroll | Arrows
Reset           : @MOD@ + Click | Backspace | Del
Host Menu       : Right-Click | Shift + F10

-- Button --
Trigger         : Click | Enter | Space
Host Menu       : Right-Click | Shift + F10

-- Parameter Lock --
Toggle Lock     : Click Label | Enter | Space (Dimmed = Locked)

Locks parameter from the "Randomize" button.

-- Tab View --
Switch View     : Click Header | Scroll on Header | Arrows

-- Preset Manager --
Browse Menu     : Click Preset Name (Center)
Cycle Presets   : Click or Keyboard Arrows
Refresh         : Select in Menu

External file changes require a Refresh. Slow network drives may cause UI pauses.

-- Info & License --
View Info       : Click "Info" | Enter | Space
Pages | Scroll  : Tab | Arrows
Copy Text       : @MOD@ + C
Close           : Click margins | Esc
)";

} // namespace Uhhyou
