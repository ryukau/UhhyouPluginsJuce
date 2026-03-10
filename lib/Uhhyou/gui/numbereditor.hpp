// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

#include <format>
#include <functional>
#include <limits>

namespace Uhhyou {

class NumberEditor : public juce::TextEditor {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberEditor)

  Uhhyou::Palette& pal;

  std::function<void(juce::String)> updateFn;

  void exitWithUpdate() {
    setVisible(false);
    updateFn(getText());
  }

public:
  NumberEditor(Palette& palette) : pal(palette), updateFn([](juce::String) {}) {}

  void invoke(juce::Component& newParent, juce::Rectangle<int> bounds, juce::String numberText,
              std::function<void(juce::String)> updateFunction) {
    newParent.addChildComponent(this, -2);
    updateFn = updateFunction;

    setBounds(bounds);
    setJustification(juce::Justification::centred);
    setSelectAllWhenFocused(true);
    setText(numberText);
    setVisible(true);
    grabKeyboardFocus();
  }

  void resized() override {
    applyFontToAllText(pal.getFont(TextSize::normal));
    juce::TextEditor::resized();
  }

  void parentSizeChanged() override {
    setBounds({0, 0, getParentWidth(), getParentHeight()});
    juce::TextEditor::parentSizeChanged();
  }

  void focusLost(FocusChangeType) override { exitWithUpdate(); }
  void escapePressed() override { setVisible(false); }
  void returnPressed() override { exitWithUpdate(); }
};

class StatusBar : public juce::TextEditor {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)

  Uhhyou::Palette& pal;

public:
  StatusBar(juce::Component& parent, Palette& palette) : pal(palette) {
    parent.addChildComponent(this);

    // Component settings.
    setVisible(true);
    setWantsKeyboardFocus(false);

    // TextEditor settings.
    setCaretVisible(false);
    setColour(outlineColourId, pal.background());
    setEscapeAndReturnKeysConsumed(false);
    setJustification(juce::Justification::centredLeft);
    setReadOnly(true);
    setScrollbarsShown(false);
    setSelectAllWhenFocused(true);
  }

  void update(const juce::String& text) { setText(text); }

  void update(const juce::RangedAudioParameter* const parameter) {
    auto text = std::format(
      "{}: {}", parameter->getName(256).toRawUTF8(),
      parameter->getText(parameter->getValue(), std::numeric_limits<float>::digits10).toRawUTF8());

    auto label = parameter->getLabel();
    if (label.isNotEmpty()) { text += std::format("[{}]", label.toRawUTF8()); }

    setText(text);
  }

  void resized() override {
    applyFontToAllText(pal.getFont(TextSize::normal));
    juce::TextEditor::resized();
  }
};

} // namespace Uhhyou
