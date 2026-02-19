// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

namespace Uhhyou {

class LookAndFeel : public juce::LookAndFeel_V4 {
private:
  Palette &pal;

public:
  LookAndFeel(Palette &palette) : pal(palette)
  {
    setColour(juce::CaretComponent::caretColourId, pal.foreground());

    setColour(juce::TextEditor::backgroundColourId, pal.background());
    setColour(juce::TextEditor::textColourId, pal.foreground());
    setColour(juce::TextEditor::highlightColourId, pal.overlayHighlight());
    setColour(juce::TextEditor::highlightedTextColourId, pal.foreground());
    setColour(juce::TextEditor::outlineColourId, pal.border());
    setColour(juce::TextEditor::focusedOutlineColourId, pal.highlightMain());

    setColour(juce::PopupMenu::backgroundColourId, pal.boxBackground());
    setColour(juce::PopupMenu::textColourId, pal.foreground());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, pal.highlightButton());
    setColour(juce::PopupMenu::highlightedTextColourId, pal.foreground());
    setColour(juce::PopupMenu::headerTextColourId, pal.foregroundInactive());

    juce::LookAndFeel::setDefaultLookAndFeel(this);
  }

  juce::Font getPopupMenuFont() override { return pal.getFont(18); }
};

} // namespace Uhhyou
