// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

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
    setText(numberText);
    setVisible(true);
    grabKeyboardFocus();
  }

  virtual void focusLost(FocusChangeType) override { exitWithUpdate(); }
  virtual void escapePressed() override { setVisible(false); }
  virtual void returnPressed() override { exitWithUpdate(); }
};

} // namespace Uhhyou
