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
#include <format>
#include <functional>
#include <limits>

namespace Uhhyou {

class ButtonAccessibilityHandler : public juce::AccessibilityHandler {
public:
  using StateModifier = std::function<juce::AccessibleState(juce::AccessibleState)>;
  using TitleCallback = std::function<juce::String()>;

  ButtonAccessibilityHandler(juce::Component& cmp, juce::AccessibilityRole role,
                             juce::AccessibilityActions act, StateModifier stateModifier = nullptr,
                             TitleCallback titleCallback = nullptr)
      : juce::AccessibilityHandler(cmp, role, std::move(act)),
        stateModifier_(std::move(stateModifier)), titleCallback_(std::move(titleCallback)) {}

  juce::AccessibleState getCurrentState() const override {
    auto state = juce::AccessibilityHandler::getCurrentState();
    if (stateModifier_) { state = stateModifier_(state); }
    return state;
  }

  juce::String getTitle() const override {
    if (titleCallback_) { return titleCallback_(); }
    return juce::AccessibilityHandler::getTitle();
  }

private:
  StateModifier stateModifier_;
  TitleCallback titleCallback_;
};

template<Style style = Style::common>
class ButtonBase : public juce::Component, public juce::SettableTooltipClient {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonBase)

protected:
  Palette& pal_;
  StatusBar& statusBar_;
  NumberEditor& numberEditor_;

  float value_{}; // Normalized in [0, 1].

  bool isMouseEntered_ = false;
  juce::Font font_;
  juce::String label_;
  juce::String hint_;

public:
  ButtonBase(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
             NumberEditor& numberEditor, const juce::String& label, const juce::String& hint)
      : pal_(palette), statusBar_(statusBar), numberEditor_(numberEditor),
        font_(juce::FontOptions{}), label_(label), hint_(hint) {
    editor.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setTitle(label_);
  }

  virtual void resized() override { font_ = pal_.getFont(TextSize::normal); }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    return std::make_unique<ButtonAccessibilityHandler>(*this, juce::AccessibilityRole::button,
                                                        juce::AccessibilityActions{});
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal_.borderWidth();
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = std::floor(static_cast<float>(getWidth()));
    const float height = std::floor(static_cast<float>(getHeight()));

    // Background.
    if constexpr (style == Style::accent) {
      ctx.setColour(value_ != 0 ? pal_.accent() : pal_.surface());
    } else if constexpr (style == Style::warning) {
      ctx.setColour(value_ != 0 ? pal_.warning() : pal_.surface());
    } else {
      ctx.setColour(value_ != 0 ? pal_.main() : pal_.surface());
    }
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    if constexpr (style == Style::accent) {
      ctx.setColour(isMouseEntered_ ? pal_.accent() : pal_.border());
    } else if constexpr (style == Style::warning) {
      ctx.setColour(isMouseEntered_ ? pal_.warning() : pal_.border());
    } else {
      ctx.setColour(isMouseEntered_ ? pal_.main() : pal_.border());
    }
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    ctx.setFont(font_);
    ctx.setColour(pal_.foreground());
    ctx.drawText(label_, juce::Rectangle<float>(float(0), float(0), width, height),
                 juce::Justification::centred);
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered_ = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered_ = false;
    statusBar_.clear();
    repaint();
  }
};

template<Style style = Style::common> class ActionButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActionButton)

  std::function<void(void)> onClick_;

  void fireAction() {
    this->value_ = 1;
    this->repaint();
    onClick_();
    juce::Timer::callAfterDelay(100, [this]() {
      this->value_ = 0;
      this->repaint();
    });
  }

public:
  ActionButton(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
               NumberEditor& numberEditor, const juce::String& label, const juce::String& hint,
               std::function<void(void)> onClick)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint),
        onClick_(onClick) {}

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [&]() { fireAction(); });

    return std::make_unique<ButtonAccessibilityHandler>(*this, juce::AccessibilityRole::button,
                                                        std::move(actions));
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    this->ButtonBase<style>::mouseEnter(event);
    this->statusBar_.setText(this->label_ + (this->hint_.isEmpty() ? "" : ". " + this->hint_));
  }

  void mouseDown(const juce::MouseEvent&) override {
    this->value_ = 1;
    this->repaint();
  }

  void mouseUp(const juce::MouseEvent&) override {
    if (this->isMouseEntered_) { onClick_(); }
    this->value_ = 0;
    this->repaint();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      fireAction();
      return true;
    }
    return false;
  }
};

template<typename Scale, Style style = Style::common>
class ToggleButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButton)

protected:
  juce::AudioProcessorEditor& editor_;
  const juce::RangedAudioParameter* const parameter_;

  Scale& scale_;
  juce::ParameterAttachment attachment_;

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

  void updateStatusBar() {
    this->statusBar_.setText(std::format("{}: {} {}", parameter_->getName(256).toRawUTF8(),
                                         this->value_ >= float(0.5) ? "ON" : "OFF",
                                         parameter_->getLabel().toRawUTF8()));
  }

  void setInternalValue(float newValue) {
    if (this->value_ == newValue) { return; }
    this->value_ = newValue;
    this->repaint();

    if (this->isMouseEntered_ || this->hasKeyboardFocus(true)) { updateStatusBar(); }

    if (auto* handler = this->getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void toggleValue() {
    auto newValue = this->value_ >= float(1) ? float(0) : float(1);
    setInternalValue(newValue);
    attachment_.setValueAsCompleteGesture(newValue >= float(1) ? float(scale_.getMax())
                                                               : float(scale_.getMin()));
  }

public:
  ToggleButton(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
               juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
               NumberEditor& numberEditor, const juce::String& label, const juce::String& hint)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint), editor_(editor),
        parameter_(parameter), scale_(scale),
        attachment_(
          *parameter,
          [&](float newRaw) {
            auto normalized = newRaw >= scale_.getMax() ? float(1) : float(0);
            setInternalValue(normalized);
          },
          undoManager) {
    attachment_.sendInitialUpdate();
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::toggle, [&]() { toggleValue(); });
    actions.addAction(juce::AccessibilityActionType::showMenu, [&]() { showHostMenuJuce(); });

    return std::make_unique<ButtonAccessibilityHandler>(
      *this, juce::AccessibilityRole::toggleButton, std::move(actions),
      [this](juce::AccessibleState state) {
        state = state.withCheckable();
        if (this->value_ >= float(1)) { state = state.withChecked(); }
        return state;
      },
      [this]() { return this->label_ + (this->value_ >= 0.5f ? " On" : " Off"); });
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    this->ButtonBase<style>::mouseEnter(event);
    updateStatusBar();
  }

  void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      showHostMenuNative(editor_.getMouseXYRelative());
      return;
    }
    toggleValue();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      toggleValue();
      return true;
    }
    if (key.isKeyCode(KP::F10Key) && key.getModifiers().isShiftDown()) {
      showHostMenuJuce();
      return true;
    }
    return false;
  }
};

template<typename Scale, Style style = Style::common>
class MomentaryButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MomentaryButton)

protected:
  juce::AudioProcessorEditor& editor_;
  const juce::RangedAudioParameter* const parameter_;

  Scale& scale_;
  juce::ParameterAttachment attachment_;

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

  void updateStatusBar() {
    this->statusBar_.setText(std::format("{}: {} {}", parameter_->getName(256).toRawUTF8(),
                                         this->value_ >= float(0.5) ? "ON" : "OFF",
                                         parameter_->getLabel().toRawUTF8()));
  }

  void setInternalValue(float newValue) {
    if (this->value_ == newValue) { return; }
    this->value_ = newValue;
    this->repaint();

    if (this->isMouseEntered_ || this->hasKeyboardFocus(true)) { updateStatusBar(); }
  }

  void setValue(bool isPressed) {
    auto newValue = isPressed ? float(1) : float(0);
    setInternalValue(newValue);
    attachment_.setValueAsCompleteGesture(isPressed ? float(scale_.getMax())
                                                    : float(scale_.getMin()));
  }

public:
  MomentaryButton(juce::AudioProcessorEditor& editor, Palette& palette,
                  juce::UndoManager* undoManager, juce::RangedAudioParameter* parameter,
                  Scale& scale, StatusBar& statusBar, NumberEditor& numberEditor,
                  const juce::String& label, const juce::String& hint)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint), editor_(editor),
        parameter_(parameter), scale_(scale),
        attachment_(
          *parameter,
          [&](float newRaw) {
            auto normalized = newRaw >= scale_.getMax() ? float(1) : float(0);
            setInternalValue(normalized);
          },
          undoManager) {
    attachment_.sendInitialUpdate();
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [&]() {
      if (this->value_ < float(1)) { setValue(true); }
      juce::Timer::callAfterDelay(100, [&]() {
        if (this->value_ > float(0)) { setValue(false); }
      });
    });
    actions.addAction(juce::AccessibilityActionType::showMenu, [&]() { showHostMenuJuce(); });

    return std::make_unique<ButtonAccessibilityHandler>(*this, juce::AccessibilityRole::button,
                                                        std::move(actions));
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    this->ButtonBase<style>::mouseEnter(event);
    updateStatusBar();
  }

  void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      showHostMenuNative(editor_.getMouseXYRelative());
      return;
    }
    setValue(true);
  }

  void mouseUp(const juce::MouseEvent&) override {
    if (this->value_ != float(0)) { setValue(false); }
  }

  bool keyStateChanged(bool) override {
    using KP = juce::KeyPress;
    if (KP::isKeyCurrentlyDown(KP::returnKey) || KP::isKeyCurrentlyDown(KP::spaceKey)) {
      if (this->value_ < float(1)) { setValue(true); }
      return true;
    }
    if (this->value_ > float(0)) {
      setValue(false);
      return true;
    }
    return false;
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::F10Key) && key.getModifiers().isShiftDown()) {
      showHostMenuJuce();
      return true;
    }
    return false;
  }

  void focusLost(juce::Component::FocusChangeType cause) override {
    using FCT = juce::Component::FocusChangeType;
    if (cause == FCT::focusChangedByTabKey || cause == FCT::focusChangedByMouseClick) {
      setValue(false);
    }
  }
};

} // namespace Uhhyou
