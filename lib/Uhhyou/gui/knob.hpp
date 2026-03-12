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
#include <limits>
#include <numbers>

namespace Uhhyou {

enum class KnobType { clamped, rotary };

template<typename Scale, KnobType knobType>
class KnobBase : public juce::Component, public juce::SettableTooltipClient {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobBase)

protected:
  class KnobValueInterface : public juce::AccessibilityValueInterface {
  public:
    explicit KnobValueInterface(KnobBase& knobToWrap) : knob_(knobToWrap) {}

    bool isReadOnly() const override { return false; }

    double getCurrentValue() const override { return double(knob_.scale_.map(knob_.value_)); }

    void setValue(double newValue) override {
      knob_.attachment_.setValueAsCompleteGesture(float(newValue));
    }

    juce::String getCurrentValueAsString() const override {
      return knob_.parameter_->getText(knob_.value_, std::numeric_limits<float>::digits10 + 1);
    }

    void setValueAsString(const juce::String& newValue) override {
      auto normalized = knob_.parameter_->getValueForText(newValue);
      knob_.attachment_.setValueAsCompleteGesture(float(knob_.scale_.map(normalized)));
    }

    AccessibleValueRange getRange() const override {
      return {{double(knob_.scale_.getMin()), double(knob_.scale_.getMax())}, 0.0};
    }

  private:
    KnobBase& knob_;
  };

  class KnobAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    explicit KnobAccessibilityHandler(KnobBase& knb, juce::AccessibilityActions act)
        : juce::AccessibilityHandler(knb, juce::AccessibilityRole::slider, std::move(act),
                                     Interfaces{std::make_unique<KnobValueInterface>(knb)}),
          knob_(knb) {}

    juce::String getTitle() const override {
      if (knob_.parameter_ != nullptr) { return knob_.parameter_->getName(256); }
      return juce::AccessibilityHandler::getTitle();
    }

  private:
    KnobBase& knob_;
  };

  juce::AudioProcessorEditor& editor_;
  const juce::RangedAudioParameter* const parameter_;

  Scale& scale_;
  Palette& pal_;
  StatusBar& statusBar_;
  NumberEditor& numberEditor_;
  juce::ParameterAttachment attachment_;

  float value_{}; // Normalized in [0, 1].
  float defaultValue_{};

  juce::Point<float> anchor_{float(0), float(0)};
  bool isMouseEntered_ = false;
  bool isMouseDragging_ = false;

  std::vector<float> snaps_;
  bool isSnapping_ = false;
  float snapDistancePixel_ = 24.0f;

  static constexpr float arcOpenPartRatio = float(1) / float(12); // In [0, 0.5].
  juce::PathStrokeType arcStrokeType_;
  juce::PathStrokeType handStrokeType_;

  void showHostMenuNative(juce::Point<int> position) {
    if (auto* hostContext = editor_.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter_)) {
        hostContextMenu->showNativeMenu(position);
      }
    }
  }

  void showHostMenuJuce() {
    if (auto* hostContext = editor_.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter_)) {
        hostContextMenu->getEquivalentPopupMenu().showMenuAsync(
          juce::PopupMenu::Options().withTargetComponent(this));
      }
    }
  }

  float getFlooredInternalValue(float normalizedValue) const {
    if (auto* scaled = dynamic_cast<const ScaledParameter<Scale>*>(parameter_)) {
      auto textRep = scaled->getTextRepresentation();
      if (textRep == ParameterTextRepresentation::raw) {
        return scale_.invmap(std::floor(scale_.map(normalizedValue)));
      } else if (textRep == ParameterTextRepresentation::normalized) {
        return std::floor(normalizedValue);
      }
    }
    return scale_.fromDisplay(std::floor(scale_.toDisplay(normalizedValue)));
  }

  // Centralized method to update value and notify accessibility.
  void setInternalValue(float newValue) {
    if (value_ == newValue) { return; }
    value_ = newValue;
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::valueChanged);
    }
  }

  juce::Point<float> mapValueToHand(float normalized, float length) {
    constexpr auto pi = std::numbers::pi_v<float>;
    auto radian = knobType == KnobType::rotary
      ? pi * 2 * normalized
      : pi * (2 * normalized - 1) * (1 - 2 * arcOpenPartRatio);
    return {-std::sin(radian) * length, std::cos(radian) * length};
  }

  void invokeTextEditor() {
    numberEditor_.invoke(*this, juce::Rectangle<int>{0, 0, getWidth(), getHeight()},
                         parameter_->getText(value_, std::numeric_limits<float>::digits10 + 1),
                         [&](juce::String text) {
                           const auto val = parameter_->getValueForText(text);
                           attachment_.setValueAsCompleteGesture(float(scale_.map(val)));
                         });
  }

  void cycleUpValue() {
    if (value_ >= float(1)) {
      setInternalValue(float(0));
      return;
    }
    auto it = std::ranges::upper_bound(snaps_, value_);
    setInternalValue((it != snaps_.end()) ? *it : float(1));
  }

  void cycleDownValue() {
    if (value_ <= 0) {
      setInternalValue(float(1));
      return;
    }
    auto it = std::ranges::lower_bound(snaps_, value_);
    setInternalValue((it != snaps_.begin()) ? *std::prev(it) : float(0));
  }

  void increaseValue(float delta) {
    float newValue = value_ + delta;
    newValue = knobType == KnobType::rotary ? (newValue - std::floor(newValue))
                                            : std::clamp(newValue, float(0), float(1));
    setInternalValue(newValue);
  }

  void increaseValueWithSnap(const juce::Point<float>& position, float deltaPixel,
                             float deltaNormalized) {
    if (isSnapping_) {
      if (std::abs(deltaPixel) <= snapDistancePixel_) { return; }
      isSnapping_ = false;
      anchor_ = position;
      return;
    }

    anchor_ = position;

    if (deltaNormalized > 0) {
      auto it = std::upper_bound(snaps_.begin(), snaps_.end(), value_);
      if (it != snaps_.end()) {
        if (value_ + deltaNormalized >= *it) {
          setInternalValue((knobType == KnobType::rotary && *it == float(1)) ? float(0) : *it);
          isSnapping_ = true;
          return;
        }
      }
    } else if (deltaNormalized < 0) {
      auto it = std::lower_bound(snaps_.begin(), snaps_.end(), value_);
      if (it != snaps_.begin()) {
        auto prev_it = std::prev(it);
        if (value_ + deltaNormalized <= *prev_it) {
          setInternalValue(*prev_it);
          isSnapping_ = true;
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

  KnobBase(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
           juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
           NumberEditor& numberEditor)
      : editor_(editor), parameter_(parameter), scale_(scale), pal_(palette), statusBar_(statusBar),
        numberEditor_(numberEditor), attachment_(
                                       *parameter,
                                       [&](float newRaw) {
                                         auto normalized = scale_.invmap(newRaw);
                                         setInternalValue(normalized);
                                       },
                                       undoManager),
        defaultValue_(parameter->getDefaultValue()),
        arcStrokeType_(palette.borderWidth() * 8.0f, juce::PathStrokeType::JointStyle::curved,
                       juce::PathStrokeType::EndCapStyle::rounded),
        handStrokeType_(palette.borderWidth() * 2.0f, juce::PathStrokeType::JointStyle::curved,
                        juce::PathStrokeType::EndCapStyle::rounded) {
    editor_.addAndMakeVisible(*this, 0);
    attachment_.sendInitialUpdate();

    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  virtual ~KnobBase() override {}

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::showMenu, [this]() { showHostMenuJuce(); });
    return std::make_unique<KnobAccessibilityHandler>(*this, std::move(actions));
  }

  void setSnaps(const std::vector<float>& source) {
    snaps_ = source;
    if (snaps_.empty()) { return; }
    std::sort(snaps_.begin(), snaps_.end());

    if constexpr (knobType == KnobType::rotary) {
      if (snaps_[0] == float(0)) { snaps_.push_back(float(1)); }
    }
  }

  void setSnapDistance(float pixels) { snapDistancePixel_ = pixels; }

  virtual void resized() override {
    arcStrokeType_.setStrokeThickness(pal_.borderWidth() * 8.0f);
    handStrokeType_.setStrokeThickness(pal_.borderWidth() * 2.0f);
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered_ = true;
    statusBar_.update(parameter_);
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered_ = false;
    statusBar_.clear();
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      showHostMenuNative(editor_.getMouseXYRelative());
      return;
    }

    if (event.mods.isMiddleButtonDown()) {
      const auto oldValue = value_;

      if (event.mods.isShiftDown()) {
        setInternalValue(getFlooredInternalValue(value_));
      } else {
        cycleUpValue();
      }

      if (value_ != oldValue) {
        attachment_.setValueAsCompleteGesture(float(scale_.map(value_)));
        statusBar_.update(parameter_);
      }
      return;
    }

    if (!event.mods.isLeftButtonDown()) { return; }

    if (event.mods.isCommandDown()) {
      setInternalValue(defaultValue_);
      attachment_.setValueAsCompleteGesture(float(scale_.map(value_)));
      statusBar_.update(parameter_);
      return;
    }

    if (liveUpdate) { attachment_.beginGesture(); }
    isMouseDragging_ = true;
    anchor_ = event.position;
    event.source.enableUnboundedMouseMovement(true);
  }

  virtual void mouseDrag(const juce::MouseEvent& event) override {
    if (!event.mods.isLeftButtonDown() || !isMouseDragging_) { return; }

    const auto sensi = event.mods.isShiftDown() ? lowSensitivity : sensitivity;
    const auto delta = (anchor_.y - event.position.y) * sensi;
    if (snaps_.empty()) {
      increaseValue(delta);
      anchor_ = event.position;
    } else {
      increaseValueWithSnap(event.position, event.position.y - anchor_.y, delta);
    }

    if (liveUpdate) {
      attachment_.setValueAsPartOfGesture(float(scale_.map(value_)));
      statusBar_.update(parameter_);
    }
    repaint();
  }

  virtual void mouseUp(const juce::MouseEvent& event) override {
    isMouseDragging_ = false;
    isSnapping_ = false;
    if (!event.mods.isLeftButtonDown()) { return; }

    event.source.enableUnboundedMouseMovement(false);

    if (liveUpdate) {
      attachment_.endGesture();
    } else {
      attachment_.setValueAsCompleteGesture(float(scale_.map(value_)));
    }

    statusBar_.update(parameter_);
    repaint();
  }

  virtual void mouseDoubleClick(const juce::MouseEvent& event) override {
    if (!event.mods.isLeftButtonDown()) { return; }
    invokeTextEditor();
  }

  virtual void mouseWheelMove(const juce::MouseEvent&,
                              const juce::MouseWheelDetails& wheel) override {
    increaseValue(wheel.deltaY * wheelSensitivity);
    attachment_.setValueAsCompleteGesture(float(scale_.map(value_)));
    statusBar_.update(parameter_);
    repaint();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    const auto& mods = key.getModifiers();

    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      invokeTextEditor();
      return true;
    }

    if (key.isKeyCode(KP::F10Key) && mods.isShiftDown()) {
      showHostMenuJuce();
      return true;
    }

    auto updateValue = [&]() {
      attachment_.setValueAsCompleteGesture(float(scale_.map(value_)));
      statusBar_.update(parameter_);
      repaint();
      return true;
    };

    if (key.isKeyCode(KP::backspaceKey) || key.isKeyCode(KP::deleteKey)) {
      if (mods.isCommandDown()) {
        setInternalValue(defaultValue_);
        return updateValue();
      } else if (mods.isShiftDown()) {
        setInternalValue(getFlooredInternalValue(value_));
        return updateValue();
      }
    }

    if (key.isKeyCode(KP::homeKey) || (key.isKeyCode(KP::upKey) && mods.isCommandDown())) {
      cycleUpValue();
      return updateValue();
    }
    if (key.isKeyCode(KP::endKey) || (key.isKeyCode(KP::downKey) && mods.isCommandDown())) {
      cycleDownValue();
      return updateValue();
    }

    const float moveSensi = mods.isShiftDown() ? lowSensitivity : sensitivity;
    if (key.isKeyCode(KP::upKey) || key.isKeyCode(KP::rightKey)) {
      increaseValue(moveSensi);
      return updateValue();
    }
    if (key.isKeyCode(KP::downKey) || key.isKeyCode(KP::leftKey)) {
      increaseValue(-moveSensi);
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
  Knob(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
       juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
       NumberEditor& numberEditor)
      : KnobBase<Scale, KnobType::clamped>(editor, palette, undoManager, parameter, scale,
                                           statusBar, numberEditor) {}

  virtual void paint(juce::Graphics& ctx) override {
    const juce::Point<int> center{this->getWidth() / 2, this->getHeight() / 2};
    ctx.setOrigin(center);

    // Arc.
    if constexpr (style == Style::accent) {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.accent()
                                          : this->pal_.border().withAlpha(0.3f));
    } else if constexpr (style == Style::warning) {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.warning()
                                          : this->pal_.border().withAlpha(0.3f));
    } else {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.main()
                                          : this->pal_.border().withAlpha(0.3f));
    }
    constexpr auto twopi = 2 * std::numbers::pi_v<float>;
    const auto radius = center.x > center.y ? center.y : center.x;
    juce::Path arc;
    arc.addCentredArc(0, 0, radius, radius, 0, twopi * (0.5f + this->arcOpenPartRatio),
                      twopi * (1.5f - this->arcOpenPartRatio), true);
    ctx.strokePath(arc, this->arcStrokeType_);

    // Mark for default value. Sharing color and style with hand.
    const float arcLineWidth = this->pal_.borderWidth() * 8.0f;
    const auto headLength = arcLineWidth / 2 - radius;
    juce::Path mark;
    mark.startNewSubPath(this->mapValueToHand(this->defaultValue_, headLength / 2));
    mark.lineTo(this->mapValueToHand(this->defaultValue_, headLength));
    ctx.strokePath(mark, this->handStrokeType_);

    // Line from center to head.
    const auto headPoint = this->mapValueToHand(this->value_, headLength);
    juce::Path hand;
    hand.startNewSubPath({0.0f, 0.0f});
    hand.lineTo(headPoint);
    ctx.setColour(this->pal_.foreground());
    ctx.strokePath(hand, this->handStrokeType_);

    // Head.
    ctx.fillEllipse(headPoint.x, headPoint.y, arcLineWidth, arcLineWidth);
  }
};

template<typename Scale, Style style = Style::common>
class RotaryKnob : public KnobBase<Scale, KnobType::rotary> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnob)

public:
  RotaryKnob(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
             juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
             NumberEditor& numberEditor)
      : KnobBase<Scale, KnobType::rotary>(editor, palette, undoManager, parameter, scale, statusBar,
                                          numberEditor) {}

  virtual void paint(juce::Graphics& ctx) override {
    const juce::Point<int> center{this->getWidth() / 2, this->getHeight() / 2};
    ctx.setOrigin(center);

    // Arc.
    if constexpr (style == Style::accent) {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.accent()
                                          : this->pal_.border().withAlpha(0.3f));
    } else if constexpr (style == Style::warning) {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.warning()
                                          : this->pal_.border().withAlpha(0.3f));
    } else {
      ctx.setColour(this->isMouseEntered_ ? this->pal_.main()
                                          : this->pal_.border().withAlpha(0.3f));
    }
    const auto radius = center.x > center.y ? center.y : center.x;
    const auto rHalf = radius / 2;
    juce::Path arc;
    arc.addEllipse(-rHalf, -rHalf, rHalf, rHalf);
    ctx.strokePath(arc, this->arcStrokeType_);

    // Mark for default value. Sharing color and style with hand.
    const float arcLineWidth = this->pal_.borderWidth() * 8.0f;
    const auto headLength = arcLineWidth / 2 - radius;
    juce::Path mark;
    mark.startNewSubPath(this->mapValueToHand(this->defaultValue_, headLength / 2));
    mark.lineTo(this->mapValueToHand(this->defaultValue_, headLength));
    ctx.strokePath(mark, this->handStrokeType_);

    // Line from center to head.
    const auto headPoint = this->mapValueToHand(this->value_, headLength);
    juce::Path hand;
    hand.startNewSubPath({0.0f, 0.0f});
    hand.lineTo(headPoint);
    ctx.setColour(this->pal_.foreground());
    ctx.strokePath(hand, this->handStrokeType_);

    // Head.
    ctx.fillEllipse(headPoint.x, headPoint.y, arcLineWidth, arcLineWidth);
  }
};

template<typename Scale, Uhhyou::Style style, KnobType knobType>
class TextKnobPainter : public KnobBase<Scale, knobType> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextKnobPainter)

  juce::Font font_;

public:
  int precision = 0;
  int32_t offset = 0;

  TextKnobPainter(juce::AudioProcessorEditor& editor, Palette& palette,
                  juce::UndoManager* undoManager, juce::RangedAudioParameter* parameter,
                  Scale& scale, StatusBar& statusBar, NumberEditor& numberEditor,
                  int precisionDigits = 0)
      : KnobBase<Scale, knobType>(editor, palette, undoManager, parameter, scale, statusBar,
                                  numberEditor),
        font_(palette.getFont(TextSize::normal)), precision(precisionDigits) {
    this->sensitivity = float(0.002);
    this->lowSensitivity = this->sensitivity / float(10);
  }

  virtual void resized() override {
    font_ = this->pal_.getFont(TextSize::normal);
    KnobBase<Scale, knobType>::resized();
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = this->pal_.borderWidth(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(this->getWidth());
    const float height = float(this->getHeight());

    const juce::Rectangle<float> bounds{lwHalf, lwHalf, width - lw1, height - lw1};

    // Background.
    ctx.setColour(this->pal_.surface());
    ctx.fillRoundedRectangle(bounds, lw2);

    // Border.
    const auto& colorBorder = [&]() {
      if constexpr (style == Uhhyou::Style::accent) {
        return this->isMouseEntered_ ? this->pal_.accent() : this->pal_.border();
      } else if constexpr (style == Uhhyou::Style::warning) {
        return this->isMouseEntered_ ? this->pal_.warning() : this->pal_.border();
      } else {
        return this->isMouseEntered_ ? this->pal_.main() : this->pal_.border();
      }
    }();
    ctx.setColour(colorBorder);
    ctx.drawRoundedRectangle(bounds, lw2, lw1);

    // Small bar indicator. It only appears on focus to avoid visual distraction.
    if (this->isMouseEntered_) {
      ctx.setColour(colorBorder.withAlpha(0.5f));
      float fillH = (height - lw1) * this->value_;
      auto fillBounds = bounds;
      fillBounds.removeFromLeft(bounds.getWidth() - 2 * lw2);
      fillBounds.removeFromTop(bounds.getHeight() - fillH);
      ctx.fillRoundedRectangle(fillBounds.reduced(1.0f), lw2);
    }

    // Text.
    ctx.setFont(font_);
    ctx.setColour(this->pal_.foreground());
    ctx.drawText(this->parameter_->getText(this->value_, int(precision)),
                 juce::Rectangle<float>(float(0), float(0), width, height),
                 juce::Justification::centred);
  }
};

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
using TextKnob = TextKnobPainter<Scale, style, KnobType::clamped>;

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
using RotaryTextKnob = TextKnobPainter<Scale, style, KnobType::rotary>;

} // namespace Uhhyou
