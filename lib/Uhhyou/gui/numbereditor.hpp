// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <limits>

namespace Uhhyou {

class NumberEditor : public juce::TextEditor {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberEditor)

  std::function<void(juce::String)> updateFn;

  void exitWithUpdate()
  {
    setVisible(false);
    updateFn(getText());
  }

public:
  NumberEditor() : updateFn([](juce::String) {}) {}

  void invoke(
    juce::Component &newParent,
    juce::Rectangle<int> bounds,
    juce::String numberText,
    std::function<void(juce::String)> updateFunction)
  {
    newParent.addChildComponent(this, -2);
    updateFn = updateFunction;

    setBounds(bounds);
    setJustification(juce::Justification::centred);
    setSelectAllWhenFocused(true);
    setText(numberText);
    setVisible(true);
    grabKeyboardFocus();
  }

  virtual void focusLost(FocusChangeType) override { exitWithUpdate(); }
  virtual void escapePressed() override { setVisible(false); }
  virtual void returnPressed() override { exitWithUpdate(); }
};

class StatusBar : public juce::TextEditor {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)

public:
  StatusBar()
  {
    setReadOnly(true);
    setCaretVisible(false);
    setEscapeAndReturnKeysConsumed(false);
    setJustification(juce::Justification::centredLeft);
    setSelectAllWhenFocused(true);
    setScrollbarsShown(false);
    setVisible(true);
  }

  void update(const juce::RangedAudioParameter *const parameter)
  {
    // TODO: Use <format>.
    auto text = parameter->getName(256);
    text += ": ";
    text
      += parameter->getText(parameter->getValue(), std::numeric_limits<float>::digits10);
    text += " ";
    text += parameter->getLabel();
    setText(text);
  }
};

} // namespace Uhhyou
