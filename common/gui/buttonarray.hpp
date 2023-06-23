// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

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

  juce::AudioProcessorEditor &editor;
  std::array<juce::RangedAudioParameter *, nParameter> parameter;
  Palette &pal;
  Scale &scale;
  ParameterArrayAttachment<nParameter> attachment;

  std::array<int, nParameter> value{};

  bool isMouseEntered = false;
  bool isTogglingOn = false;
  juce::Point<float> mousePos;

  inline int getMousePosIndex() { return int((mousePos.x * value.size()) / getWidth()); }

  inline float getRawValue(int index)
  {
    return value[index] == 0 ? float(scale.getMin()) : float(scale.getMax());
  }

  inline void toggleValueAtIndex(int index)
  {
    value[index] = isTogglingOn ? 1 : 0;
    attachment.beginGesture(index);
    attachment.setValueAsPartOfGesture(index, getRawValue(index));
  }

public:
  ButtonArray(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    std::array<juce::RangedAudioParameter *, nParameter> parameter,
    Scale &scale)
    : editor(editor)
    , parameter(parameter)
    , scale(scale)
    , pal(palette)
    , attachment(
        parameter,
        [&](int index, float rawValue) {
          if (index < 0 && index >= value.size()) return;
          auto newValue = rawValue <= scale.getMin() ? 0 : 1;
          if (value[index] != newValue) {
            value[index] = newValue;
            repaint();
          }
        },
        undoManager)
  {
    attachment.sendInitialUpdate();
  }

  virtual ~ButtonArray() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());
    const float innerWidth = width - lw2;
    const float innerHeight = height - lw2;

    // Background and border.
    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);
    ctx.setColour(pal.foreground());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Buttons.
    const float buttonWidth = innerWidth / value.size();
    ctx.setColour(pal.highlightMain());
    for (size_t idx = 0; idx < value.size(); ++idx) {
      if (value[idx] != 0)
        ctx.fillRoundedRectangle(
          idx * buttonWidth + lw2, lw2, buttonWidth - lw2, innerHeight - lw2, lw2);
    }

    // Highlight.
    const size_t valueIndex = getMousePosIndex();
    if (isMouseEntered && valueIndex < value.size()) {
      ctx.setColour(pal.overlayHighlight());
      ctx.fillRoundedRectangle(
        valueIndex * buttonWidth + lw1, lw1, buttonWidth, innerHeight, lw2);
    }
  }

  void mouseMove(const juce::MouseEvent &event) override
  {
    mousePos = event.position;
    repaint();
  }

  void mouseEnter(const juce::MouseEvent &event) override
  {
    isMouseEntered = true;
    mousePos = event.position;
    repaint();
  }

  void mouseExit(const juce::MouseEvent &event) override
  {
    isMouseEntered = false;
    mousePos = event.position;
    repaint();
  }

  void mouseDown(const juce::MouseEvent &event) override
  {
    mousePos = event.position;

    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor.getHostContext();
      if (hostContext == nullptr) return;

      auto index = getMousePosIndex();
      if (index >= value.size()) return;

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter[index]);
      if (hostContextMenu == nullptr) return;

      hostContextMenu->showNativeMenu(editor.getMouseXYRelative());
      return;
    }

    if (!event.mods.isLeftButtonDown()) return;

    const auto index = getMousePosIndex();
    if (index < value.size()) {
      isTogglingOn = value[index] == 0;
      toggleValueAtIndex(index);
    }
    repaint();
  }

  void mouseDrag(const juce::MouseEvent &event) override
  {
    if (!event.mods.isLeftButtonDown()) return;
    mousePos = event.position;
    const auto index = getMousePosIndex();
    if (index < value.size()) toggleValueAtIndex(index);
    repaint();
  }

  void mouseUp(const juce::MouseEvent &) override { attachment.endGesture(); }

  void mouseDoubleClick(const juce::MouseEvent &) override {}

  void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
  {
    const auto index = getMousePosIndex();
    if (index < value.size() && wheel.deltaY != 0) {
      value[index] = value[index] == 0 ? 1 : 0;
      attachment.setValueAsCompleteGesture(index, getRawValue(index));
    }
    repaint();
  }
};

} // namespace Uhhyou
