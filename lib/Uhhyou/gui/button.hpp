// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "./numbereditor.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace Uhhyou {

template<Style style = Style::common> class ButtonBase : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonBase)

protected:
  Palette &pal;
  NumberEditor &textInput;

  float value{}; // Normalized in [0, 1].

  bool isMouseEntered = false;
  juce::Font font;
  juce::String label;

public:
  ButtonBase(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    NumberEditor &textInput,
    const juce::String &label)
    : pal(palette), textInput(textInput), font(juce::FontOptions{}), label(label)
  {
    editor.addAndMakeVisible(*this, 0);
  }

  virtual ~ButtonBase() override {}

  virtual void resized() override { font = pal.getFont(pal.textSizeUi()); }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());

    // Background.
    if constexpr (style == Style::accent) {
      ctx.setColour(value ? pal.highlightAccent() : pal.boxBackground());
    } else if constexpr (style == Style::warning) {
      ctx.setColour(value ? pal.highlightWarning() : pal.boxBackground());
    } else {
      ctx.setColour(value ? pal.highlightButton() : pal.boxBackground());
    }
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    if constexpr (style == Style::accent) {
      ctx.setColour(isMouseEntered ? pal.highlightAccent() : pal.border());
    } else if constexpr (style == Style::warning) {
      ctx.setColour(isMouseEntered ? pal.highlightWarning() : pal.border());
    } else {
      ctx.setColour(isMouseEntered ? pal.highlightButton() : pal.border());
    }
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    ctx.setFont(font);
    ctx.setColour(pal.foreground());
    ctx.drawText(
      label, juce::Rectangle<float>(float(0), float(0), width, height),
      juce::Justification::centred);
  }

  virtual void mouseMove(const juce::MouseEvent &) override {}
  virtual void mouseDrag(const juce::MouseEvent &) override {}
  virtual void mouseDown(const juce::MouseEvent &) override {}
  virtual void mouseUp(const juce::MouseEvent &) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override
  {
  }

  virtual void mouseEnter(const juce::MouseEvent &) override
  {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent &) override
  {
    isMouseEntered = false;
    repaint();
  }
};

template<Style style = Style::common> class ActionButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActionButton)

  std::function<void(void)> onClick;

public:
  ActionButton(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    NumberEditor &textInput,
    const juce::String &label,
    std::function<void(void)> onClick)
    : ButtonBase<style>(editor, palette, textInput, label), onClick(onClick)
  {
  }

  virtual ~ActionButton() override {}

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    this->value = 1;
    this->repaint();
  }

  virtual void mouseUp(const juce::MouseEvent &) override
  {
    if (this->isMouseEntered) onClick();
    this->value = 0;
    this->repaint();
  }
};

template<typename Scale, Style style = Style::common>
class ToggleButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButton)

protected:
  juce::AudioProcessorEditor &editor;
  const juce::RangedAudioParameter *const parameter;

  Scale &scale;
  juce::ParameterAttachment attachment;

  float defaultValue{};

  inline void toggleValue()
  {
    this->value = this->value >= float(1) ? float(0) : float(1);
    attachment.setValueAsCompleteGesture(
      this->value >= float(1) ? float(scale.getMax()) : float(scale.getMin()));
  }

public:
  ToggleButton(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    NumberEditor &textInput,
    const juce::String &label)
    : ButtonBase<style>(editor, palette, textInput, label)
    , editor(editor)
    , parameter(parameter)
    , scale(scale)
    , attachment(
        *parameter,
        [&](float newRaw) {
          auto normalized = newRaw >= scale.getMax() ? float(1) : float(0);
          if (this->value == normalized) return;
          this->value = normalized;
          this->repaint();
        },
        undoManager)
    , defaultValue(parameter->getDefaultValue())
  {
    attachment.sendInitialUpdate();
  }

  virtual ~ToggleButton() override {}

  virtual void mouseDown(const juce::MouseEvent &event) override
  {
    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor.getHostContext();
      if (hostContext == nullptr) return;

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter);
      if (hostContextMenu == nullptr) return;

      hostContextMenu->showNativeMenu(editor.getMouseXYRelative());
      return;
    }

    toggleValue();
    this->repaint();
  }

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
  {
    if (std::abs(wheel.deltaY) <= std::numeric_limits<float>::epsilon()) toggleValue();
    this->repaint();
  }
};

} // namespace Uhhyou
