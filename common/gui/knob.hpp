// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace Uhhyou {

template<typename Scale> class KnobBase : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobBase)

protected:
  juce::AudioProcessorEditor &editor;
  const juce::RangedAudioParameter *const parameter;

  Palette &pal;
  Scale &scale;
  juce::ParameterAttachment attachment;

  float value{}; // Normalized in [0, 1].
  float defaultValue{};

  juce::Point<float> anchor{float(0), float(0)};
  bool isMouseEntered = false;

  static constexpr float arcOpenPartRatio = float(1) / float(12); // In [0, 0.5].
  juce::PathStrokeType arcStrokeType;
  juce::PathStrokeType handStrokeType;

  juce::Point<float> mapValueToHand(float normalized, float length)
  {
    constexpr auto pi = std::numbers::pi_v<float>;
    auto radian = pi * (2 * normalized - 1) * (1 - 2 * arcOpenPartRatio);
    return {-std::sin(radian) * length, std::cos(radian) * length};
  }

public:
  bool liveUpdate = true;           // When false, only update value on mouse up event.
  float sensitivity = float(0.004); // MovedPixel * sensitivity = valueChanged.
  float lowSensitivity = float(sensitivity / 5);
  float wheelSensitivity = float(sensitivity / 10);

  KnobBase(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale)
    : editor(editor)
    , parameter(parameter)
    , scale(scale)
    , pal(palette)
    , attachment(
        *parameter,
        [&](float newRaw) {
          auto normalized = scale.invmap(newRaw);
          if (value == normalized) return;
          value = normalized;
          repaint();
        },
        undoManager)
    , defaultValue(parameter->getDefaultValue())
    , arcStrokeType(
        palette.borderThick(),
        juce::PathStrokeType::JointStyle::curved,
        juce::PathStrokeType::EndCapStyle::rounded)
    , handStrokeType(
        palette.borderThick() / 4,
        juce::PathStrokeType::JointStyle::curved,
        juce::PathStrokeType::EndCapStyle::rounded)
  {
    editor.addAndMakeVisible(*this, 0);
    attachment.sendInitialUpdate();
  }

  virtual ~KnobBase() override {}

  virtual void resized() override
  {
    arcStrokeType.setStrokeThickness(pal.borderThick());
    handStrokeType.setStrokeThickness(pal.borderThick() / 4);
  }

  virtual void mouseMove(const juce::MouseEvent &) override {}

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

    if (event.mods.isMiddleButtonDown()) {
      const auto oldValue = value;

      if (event.mods.isShiftDown()) {
        value = this->scale.fromDisplay(std::floor(this->scale.toDisplay(value)));
      } else {
        const auto &mid = defaultValue;
        value = value < mid ? mid : value < float(1) ? float(1) : float(0);
      }

      if (value != oldValue) {
        this->attachment.setValueAsCompleteGesture(float(this->scale.map(value)));
      }
      repaint();
      return;
    }

    if (!event.mods.isLeftButtonDown()) return;

    if (event.mods.isCommandDown()) {
      value = defaultValue;
      attachment.setValueAsCompleteGesture(float(scale.map(value)));
      repaint();
      return;
    }

    if (liveUpdate) attachment.beginGesture();
    anchor = event.position;
    event.source.enableUnboundedMouseMovement(true);
  }

  virtual void mouseDrag(const juce::MouseEvent &event) override
  {
    if (!event.mods.isLeftButtonDown()) return;

    const auto sensi = event.mods.isShiftDown() ? lowSensitivity : sensitivity;
    value = std::clamp(value + (anchor.y - event.position.y) * sensi, 0.0f, 1.0f);

    if (liveUpdate) attachment.setValueAsPartOfGesture(float(scale.map(value)));
    repaint();

    anchor = event.position;
  }

  virtual void mouseUp(const juce::MouseEvent &event) override
  {
    if (!event.mods.isLeftButtonDown()) return;

    event.source.enableUnboundedMouseMovement(false);

    if (liveUpdate)
      attachment.endGesture();
    else
      attachment.setValueAsCompleteGesture(float(scale.map(value)));
  }

  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
  {
    value = std::clamp(value + wheel.deltaY * wheelSensitivity, 0.0f, 1.0f);
    attachment.setValueAsCompleteGesture(float(scale.map(value)));
    repaint();
  }
};

template<typename Scale, Style style = Style::common>
class Knob : public KnobBase<Scale> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)

public:
  Knob(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale)
    : KnobBase<Scale>(editor, palette, undoManager, parameter, scale)
  {
  }

  virtual ~Knob() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    const juce::Point<int> center{this->getWidth() / 2, this->getHeight() / 2};
    ctx.setOrigin(center);

    const float arcLineWidth = this->pal.borderThick();

    // Arc.
    const auto radius = center.x > center.y ? center.y : center.x;
    if constexpr (style == Style::accent) {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightAccent() : this->pal.unfocused());
    } else if constexpr (style == Style::warning) {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightWarning() : this->pal.unfocused());
    } else {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightMain() : this->pal.unfocused());
    }
    constexpr auto twopi = 2 * std::numbers::pi_v<float>;
    juce::Path arc;
    arc.addCentredArc(
      0, 0, radius - this->arcLineWidth / 2, radius - this->arcLineWidth / 2, 0,
      twopi * (0.5f + this->arcOpenPartRatio), twopi * (1.5f - this->arcOpenPartRatio),
      true);
    ctx.strokePath(arc, this->arcStrokeType);

    // Mark for default value. Sharing color and style with hand.
    constexpr auto arcHalf = this->arcLineWidth / 2;
    const auto headLength = arcHalf - radius;
    juce::Path mark;
    mark.startNewSubPath(this->mapValueToHand(this->defaultValue, headLength / 2));
    mark.lineTo(this->mapValueToHand(this->defaultValue, headLength));
    ctx.strokePath(mark, this->handStrokeType);

    // Line from center to head.
    const auto headPoint = this->mapValueToHand(this->value, headLength);
    juce::Path hand;
    hand.startNewSubPath({0.0f, 0.0f});
    hand.lineTo(headPoint);
    ctx.setColour(this->pal.foreground());
    ctx.strokePath(hand, this->handStrokeType);

    // Head.
    ctx.fillEllipse(
      headPoint.x - arcHalf, headPoint.y - arcHalf, this->arcLineWidth,
      this->arcLineWidth);
  }
};

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
class TextKnob : public KnobBase<Scale> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextKnob)

  juce::Font font;

public:
  uint32_t precision = 0;
  int32_t offset = 0;

  TextKnob(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    uint32_t precision = 0)
    : KnobBase<Scale>(editor, palette, undoManager, parameter, scale)
    , font(palette.getFont(palette.textSizeUi()))
    , precision(precision)
  {
    this->sensitivity = float(0.002);
    this->lowSensitivity = this->sensitivity / float(10);
  }

  virtual ~TextKnob() override {}

  virtual void resized() override
  {
    font = this->pal.getFont(this->pal.textSizeUi());

    KnobBase<Scale>::resized();
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = this->pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(this->getWidth());
    const float height = float(this->getHeight());

    // Background.
    ctx.setColour(this->pal.boxBackground());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    ctx.setColour(this->pal.boxBackground());
    ctx.drawRect(float(0), float(0), width, height);
    if constexpr (style == Uhhyou::Style::accent) {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightAccent() : this->pal.border());
    } else if constexpr (style == Uhhyou::Style::warning) {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightWarning() : this->pal.border());
    } else {
      ctx.setColour(
        this->isMouseEntered ? this->pal.highlightMain() : this->pal.border());
    }
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    ctx.setFont(font);
    ctx.setColour(this->pal.foreground());
    ctx.drawText(
      this->parameter->getText(this->value, precision),
      juce::Rectangle<float>(float(0), float(0), width, height),
      juce::Justification::centred);
  }
};

} // namespace Uhhyou
