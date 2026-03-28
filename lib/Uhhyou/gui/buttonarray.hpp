// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "parameterarrayattachment.hpp"
#include "style.hpp"

#include <algorithm>
#include <limits>

namespace Uhhyou {

template<typename Scale, size_t nParameter> class ButtonArray : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonArray)

  juce::AudioProcessorEditor& editor_;
  std::array<juce::RangedAudioParameter*, nParameter> parameter_;
  Palette& pal_;
  Scale& scale_;
  ParameterArrayAttachment<nParameter> attachment_;

  std::array<int, nParameter> value_{};

  bool isMouseEntered_ = false;
  bool isTogglingOn_ = false;
  juce::Point<float> mousePos_;

  inline int getMousePosIndex() { return int((mousePos_.x * value_.size()) / getWidth()); }

  inline float getRawValue(int index) {
    return value_[index] == 0 ? float(scale_.getMin()) : float(scale_.getMax());
  }

  inline void toggleValueAtIndex(int index) {
    value_[index] = isTogglingOn_ ? 1 : 0;
    attachment_.beginGesture(index);
    attachment_.setValueAsPartOfGesture(index, getRawValue(index));
  }

public:
  ButtonArray(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
              std::array<juce::RangedAudioParameter*, nParameter> parameter, Scale& scale)
      : editor_(editor), parameter_(parameter), scale_(scale), pal_(palette),
        attachment_(
          parameter,
          [&](int index, float rawValue) {
            if (index < 0 && index >= value_.size()) { return; }
            auto newValue = rawValue <= scale_.getMin() ? 0 : 1;
            if (value_[index] != newValue) {
              value_[index] = newValue;
              repaint();
            }
          },
          undoManager) {
    attachment_.sendInitialUpdate();
  }

  virtual ~ButtonArray() override {}

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal_.borderWidth();
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());
    const float innerWidth = width - lw2;
    const float innerHeight = height - lw2;

    // Background and border.
    juce::Colour bgColour = pal_.surface();
    ctx.setColour(bgColour);
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);
    ctx.setColour(pal_.getForeground(bgColour));
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Buttons.
    const float buttonWidth = innerWidth / value_.size();
    ctx.setColour(pal_.main());
    for (size_t idx = 0; idx < value_.size(); ++idx) {
      if (value_[idx] != 0) {
        ctx.fillRoundedRectangle(idx * buttonWidth + lw2, lw2, buttonWidth - lw2, innerHeight - lw2,
                                 lw2);
      }
    }

    // Highlight.
    const size_t valueIndex = getMousePosIndex();
    if (isMouseEntered_ && valueIndex < value_.size()) {
      ctx.setColour(pal_.main().withAlpha(0.25f));
      ctx.fillRoundedRectangle(valueIndex * buttonWidth + lw1, lw1, buttonWidth, innerHeight, lw2);
    }
  }

  void mouseMove(const juce::MouseEvent& event) override {
    mousePos_ = event.position;
    repaint();
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    isMouseEntered_ = true;
    mousePos_ = event.position;
    repaint();
  }

  void mouseExit(const juce::MouseEvent& event) override {
    isMouseEntered_ = false;
    mousePos_ = event.position;
    repaint();
  }

  void mouseDown(const juce::MouseEvent& event) override {
    mousePos_ = event.position;

    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor_.getHostContext();
      if (hostContext == nullptr) { return; }

      auto index = getMousePosIndex();
      if (index >= value_.size()) { return; }

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter_[index]);
      if (hostContextMenu == nullptr) { return; }

      hostContextMenu->showNativeMenu(editor_.getMouseXYRelative());
      return;
    }

    if (!event.mods.isLeftButtonDown()) { return; }

    const auto index = getMousePosIndex();
    if (index < value_.size()) {
      isTogglingOn_ = value_[index] == 0;
      toggleValueAtIndex(index);
    }
    repaint();
  }

  void mouseDrag(const juce::MouseEvent& event) override {
    if (!event.mods.isLeftButtonDown()) { return; }
    mousePos_ = event.position;
    const auto index = getMousePosIndex();
    if (index < value_.size()) { toggleValueAtIndex(index); }
    repaint();
  }

  void mouseUp(const juce::MouseEvent&) override { attachment_.endGesture(); }

  void mouseDoubleClick(const juce::MouseEvent&) override {}

  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override {
    const auto index = getMousePosIndex();
    if (index < value_.size() && wheel.deltaY != 0) {
      value_[index] = value_[index] == 0 ? 1 : 0;
      attachment_.setValueAsCompleteGesture(index, getRawValue(index));
    }
    repaint();
  }
};

} // namespace Uhhyou
