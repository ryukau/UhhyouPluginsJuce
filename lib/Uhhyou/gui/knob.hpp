// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "numbereditor.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>

namespace Uhhyou {

enum class KnobType { clamped, rotary };

template<typename Scale, KnobType knobType> class KnobBase : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobBase)

protected:
  juce::AudioProcessorEditor &editor;
  const juce::RangedAudioParameter *const parameter;

  Scale &scale;
  Palette &pal;
  StatusBar &statusBar;
  NumberEditor &numberEditor;
  juce::ParameterAttachment attachment;

  float value{}; // Normalized in [0, 1].
  float defaultValue{};

  juce::Point<float> anchor{float(0), float(0)};
  bool isMouseEntered = false;
  bool isMouseDragging = false;

  std::vector<float> snaps;
  bool isSnapping = false;
  float snapDistancePixel = 24.0f;

  static constexpr float arcOpenPartRatio = float(1) / float(12); // In [0, 0.5].
  juce::PathStrokeType arcStrokeType;
  juce::PathStrokeType handStrokeType;

  juce::Point<float> mapValueToHand(float normalized, float length)
  {
    constexpr auto pi = std::numbers::pi_v<float>;
    auto radian = knobType == KnobType::rotary
      ? pi * 2 * normalized
      : pi * (2 * normalized - 1) * (1 - 2 * arcOpenPartRatio);
    return {-std::sin(radian) * length, std::cos(radian) * length};
  }

  void invokeTextEditor()
  {
    numberEditor.invoke(
      *this, juce::Rectangle<int>{0, 0, getWidth(), getHeight()},
      this->parameter->getText(this->value, std::numeric_limits<float>::digits10 + 1),
      [&](juce::String text)
      {
        const auto value = parameter->getValueForText(text);
        attachment.setValueAsCompleteGesture(float(this->scale.map(value)));
      });
  }

  void cycleUpValue()
  {
    if (value >= float(1)) {
      value = float(0);
      return;
    }
    auto it = std::ranges::upper_bound(snaps, value);
    value = (it != snaps.end()) ? *it : float(1);
  }

  void cycleDownValue()
  {
    if (value <= 0) {
      value = float(1);
      return;
    }
    auto it = std::ranges::lower_bound(snaps, value);
    value = (it != snaps.begin()) ? *std::prev(it) : float(0);
  }

  void increaseValue(float delta)
  {
    value += delta;
    value = knobType == KnobType::rotary ? (value - std::floor(value))
                                         : std::clamp(value, float(0), float(1));
  }

  void increaseValueWithSnap(
    const juce::Point<float> &position, float deltaPixel, float deltaNormalized)
  {
    if (isSnapping) {
      if (std::abs(deltaPixel) <= snapDistancePixel) return;
      isSnapping = false;
      anchor = position;
      return;
    }

    anchor = position;

    if (deltaNormalized > 0) {
      auto it = std::upper_bound(snaps.begin(), snaps.end(), value);
      if (it != snaps.end()) {
        if (value + deltaNormalized >= *it) {
          value = (knobType == KnobType::rotary && *it == float(1)) ? float(0) : *it;
          isSnapping = true;
          return;
        }
      }
    } else if (deltaNormalized < 0) {
      auto it = std::lower_bound(snaps.begin(), snaps.end(), value);
      if (it != snaps.begin()) {
        auto prev_it = std::prev(it);
        if (value + deltaNormalized <= *prev_it) {
          value = *prev_it;
          isSnapping = true;
          return;
        }
      }
    }

    increaseValue(deltaNormalized);
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
    Scale &scale,
    StatusBar &statusBar,
    NumberEditor &numberEditor)
    : editor(editor)
    , parameter(parameter)
    , scale(scale)
    , pal(palette)
    , statusBar(statusBar)
    , numberEditor(numberEditor)
    , attachment(
        *parameter,
        [&](float newRaw)
        {
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

    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  virtual ~KnobBase() override {}

  void setSnaps(const std::vector<float> &snaps_)
  {
    snaps = snaps_;
    if (snaps.empty()) return;
    std::sort(snaps.begin(), snaps.end());

    if constexpr (knobType == KnobType::rotary) {
      if (snaps[0] == float(0)) snaps.push_back(float(1));
    }
  }

  void setSnapDistance(float pixels) { snapDistancePixel = pixels; }

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
        cycleUpValue();
      }

      if (value != oldValue) {
        this->attachment.setValueAsCompleteGesture(float(this->scale.map(value)));
        statusBar.update(parameter);
      }
      repaint();
      return;
    }

    if (!event.mods.isLeftButtonDown()) return;

    if (event.mods.isCommandDown()) {
      value = defaultValue;
      attachment.setValueAsCompleteGesture(float(scale.map(value)));
      statusBar.update(parameter);
      repaint();
      return;
    }

    if (liveUpdate) attachment.beginGesture();
    isMouseDragging = true;
    anchor = event.position;
    event.source.enableUnboundedMouseMovement(true);
  }

  virtual void mouseDrag(const juce::MouseEvent &event) override
  {
    if (!event.mods.isLeftButtonDown() || !isMouseDragging) return;

    const auto sensi = event.mods.isShiftDown() ? lowSensitivity : sensitivity;
    const auto delta = (anchor.y - event.position.y) * sensi;
    if (snaps.empty()) {
      increaseValue(delta);
      anchor = event.position;
    } else {
      increaseValueWithSnap(event.position, event.position.y - anchor.y, delta);
    }

    if (liveUpdate) {
      attachment.setValueAsPartOfGesture(float(scale.map(value)));
      statusBar.update(parameter);
    }
    repaint();
  }

  virtual void mouseUp(const juce::MouseEvent &event) override
  {
    isMouseDragging = false;
    isSnapping = false;
    if (!event.mods.isLeftButtonDown()) return;

    event.source.enableUnboundedMouseMovement(false);

    if (liveUpdate) {
      attachment.endGesture();
    } else {
      attachment.setValueAsCompleteGesture(float(scale.map(value)));
    }

    statusBar.update(parameter);
    repaint();
  }

  virtual void mouseDoubleClick(const juce::MouseEvent &event) override
  {
    if (!event.mods.isLeftButtonDown()) return;
    invokeTextEditor();
  }

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
  {
    increaseValue(wheel.deltaY * wheelSensitivity);
    attachment.setValueAsCompleteGesture(float(scale.map(value)));
    statusBar.update(parameter);
    repaint();
  }

  bool keyPressed(const juce::KeyPress &key) override
  {
    using KP = juce::KeyPress;
    const auto &mods = key.getModifiers();

    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      invokeTextEditor();
      return true;
    }

    auto updateValue = [&]()
    {
      attachment.setValueAsCompleteGesture(float(scale.map(value)));
      statusBar.update(parameter);
      repaint();
      return true;
    };

    if (key.isKeyCode(KP::homeKey) || (key.isKeyCode(KP::upKey) && mods.isCommandDown()))
    {
      cycleUpValue();
      return updateValue();
    }
    if (key.isKeyCode(KP::endKey) || (key.isKeyCode(KP::downKey) && mods.isCommandDown()))
    {
      cycleDownValue();
      return updateValue();
    }

    if (key.isKeyCode(KP::upKey)) {
      increaseValue(sensitivity);
      return updateValue();
    }
    if (key.isKeyCode(KP::downKey)) {
      increaseValue(-sensitivity);
      return updateValue();
    }

    return false;
  }
};

template<typename Scale, Style style = Style::common>
class Knob : public KnobBase<Scale, KnobType::clamped> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)

public:
  Knob(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    StatusBar &statusBar,
    NumberEditor &numberEditor)
    : KnobBase<Scale, KnobType::clamped>(
        editor, palette, undoManager, parameter, scale, statusBar, numberEditor)
  {
  }

  virtual ~Knob() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    const juce::Point<int> center{this->getWidth() / 2, this->getHeight() / 2};
    ctx.setOrigin(center);

    // Arc.
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
    const auto radius = center.x > center.y ? center.y : center.x;
    juce::Path arc;
    arc.addCentredArc(
      0, 0, radius, radius, 0, twopi * (0.5f + this->arcOpenPartRatio),
      twopi * (1.5f - this->arcOpenPartRatio), true);
    ctx.strokePath(arc, this->arcStrokeType);

    // Mark for default value. Sharing color and style with hand.
    const float arcLineWidth = this->pal.borderThick();
    const auto headLength = arcLineWidth / 2 - radius;
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
    ctx.fillEllipse(headPoint.x, headPoint.y, arcLineWidth, arcLineWidth);
  }
};

template<typename Scale, Style style = Style::common>
class RotaryKnob : public KnobBase<Scale, KnobType::rotary> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnob)

public:
  RotaryKnob(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    StatusBar &statusBar,
    NumberEditor &numberEditor)
    : KnobBase<Scale, KnobType::rotary>(
        editor, palette, undoManager, parameter, scale, statusBar, numberEditor)
  {
  }

  virtual ~RotaryKnob() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    const juce::Point<int> center{this->getWidth() / 2, this->getHeight() / 2};
    ctx.setOrigin(center);

    // Arc.
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
    const auto radius = center.x > center.y ? center.y : center.x;
    const auto rHalf = radius / 2;
    juce::Path arc;
    arc.addEllipse(-rHalf, -rHalf, rHalf, rHalf);
    ctx.strokePath(arc, this->arcStrokeType);

    // Mark for default value. Sharing color and style with hand.
    const float arcLineWidth = this->pal.borderThick();
    const auto headLength = arcLineWidth / 2 - radius;
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
    ctx.fillEllipse(headPoint.x, headPoint.y, arcLineWidth, arcLineWidth);
  }
};

template<typename Scale, Uhhyou::Style style, KnobType knobType>
class TextKnobPainter : public KnobBase<Scale, knobType> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextKnobPainter)

  juce::Font font;

public:
  int precision = 0;
  int32_t offset = 0;

  TextKnobPainter(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    StatusBar &statusBar,
    NumberEditor &numberEditor,
    int precision = 0)
    : KnobBase<Scale, knobType>(
        editor, palette, undoManager, parameter, scale, statusBar, numberEditor)
    , font(palette.getFont(palette.textSizeUi()))
    , precision(precision)
  {
    this->sensitivity = float(0.002);
    this->lowSensitivity = this->sensitivity / float(10);
  }

  virtual ~TextKnobPainter() override {}

  virtual void resized() override
  {
    font = this->pal.getFont(this->pal.textSizeUi());

    KnobBase<Scale, knobType>::resized();
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = this->pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(this->getWidth());
    const float height = float(this->getHeight());

    const juce::Rectangle<float> bounds{lwHalf, lwHalf, width - lw1, height - lw1};

    // Background.
    ctx.setColour(this->pal.boxBackground());
    ctx.fillRoundedRectangle(bounds, lw2);

    // Border.
    const auto &colorBorder = [&]()
    {
      if constexpr (style == Uhhyou::Style::accent) {
        return this->isMouseEntered ? this->pal.highlightAccent() : this->pal.border();
      } else if constexpr (style == Uhhyou::Style::warning) {
        return this->isMouseEntered ? this->pal.highlightWarning() : this->pal.border();
      } else {
        return this->isMouseEntered ? this->pal.highlightMain() : this->pal.border();
      }
    }();
    ctx.setColour(colorBorder);
    ctx.drawRoundedRectangle(bounds, lw2, lw1);

    // Small bar indicator. It only appears on focus to avoid visual distraction.
    if (this->isMouseEntered) {
      ctx.setColour(colorBorder.withAlpha(0.5f));
      float fillH = (height - lw1) * this->value;
      auto fillBounds = bounds;
      fillBounds.removeFromLeft(bounds.getWidth() - 2 * lw2);
      fillBounds.removeFromTop(bounds.getHeight() - fillH);
      ctx.fillRoundedRectangle(fillBounds.reduced(1.0f), lw2);
    }

    // Text.
    ctx.setFont(font);
    ctx.setColour(this->pal.foreground());
    ctx.drawText(
      this->parameter->getText(this->value, int(precision)),
      juce::Rectangle<float>(float(0), float(0), width, height),
      juce::Justification::centred);
  }
};

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
using TextKnob = TextKnobPainter<Scale, style, KnobType::clamped>;

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
using RotaryTextKnob = TextKnobPainter<Scale, style, KnobType::rotary>;

} // namespace Uhhyou
