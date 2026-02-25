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

  ButtonAccessibilityHandler(juce::Component& comp, juce::AccessibilityRole role,
                             juce::AccessibilityActions actions,
                             StateModifier stateModifier = nullptr,
                             TitleCallback titleCallback = nullptr)
      : juce::AccessibilityHandler(comp, role, std::move(actions)),
        stateModifier(std::move(stateModifier)), titleCallback(std::move(titleCallback)) {}

  juce::AccessibleState getCurrentState() const override {
    auto state = juce::AccessibilityHandler::getCurrentState();
    if (stateModifier) { state = stateModifier(state); }
    return state;
  }

  juce::String getTitle() const override {
    if (titleCallback) { return titleCallback(); }
    return juce::AccessibilityHandler::getTitle();
  }

private:
  StateModifier stateModifier;
  TitleCallback titleCallback;
};

template<Style style = Style::common>
class ButtonBase : public juce::Component, public juce::SettableTooltipClient {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonBase)

protected:
  Palette& pal;
  StatusBar& statusBar;
  NumberEditor& numberEditor;

  float value{}; // Normalized in [0, 1].

  bool isMouseEntered = false;
  juce::Font font;
  juce::String label;
  juce::String hint;

public:
  ButtonBase(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
             NumberEditor& numberEditor, const juce::String& label, const juce::String& hint)
      : pal(palette), statusBar(statusBar), numberEditor(numberEditor), font(juce::FontOptions{}),
        label(label), hint(hint) {
    editor.addAndMakeVisible(*this, 0);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
    setTitle(label);
  }

  virtual void resized() override { font = pal.getFont(pal.textSizeUi()); }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    return std::make_unique<ButtonAccessibilityHandler>(*this, juce::AccessibilityRole::button,
                                                        juce::AccessibilityActions{});
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal.borderThin();
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
    ctx.drawText(label, juce::Rectangle<float>(float(0), float(0), width, height),
                 juce::Justification::centred);
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    statusBar.clear();
    repaint();
  }
};

template<Style style = Style::common> class ActionButton : public ButtonBase<style> {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActionButton)

  std::function<void(void)> onClick;

  void fireAction() {
    this->value = 1;
    this->repaint();
    onClick();
    juce::Timer::callAfterDelay(100, [this]() {
      this->value = 0;
      this->repaint();
    });
  }

public:
  ActionButton(juce::AudioProcessorEditor& editor, Palette& palette, StatusBar& statusBar,
               NumberEditor& numberEditor, const juce::String& label, const juce::String& hint,
               std::function<void(void)> onClick)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint), onClick(onClick) {
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [&]() { fireAction(); });

    return std::make_unique<ButtonAccessibilityHandler>(*this, juce::AccessibilityRole::button,
                                                        std::move(actions));
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    this->ButtonBase<style>::mouseEnter(event);
    this->statusBar.setText(this->label + (this->hint.isEmpty() ? "" : ". " + this->hint));
  }

  void mouseDown(const juce::MouseEvent&) override {
    this->value = 1;
    this->repaint();
  }

  void mouseUp(const juce::MouseEvent&) override {
    if (this->isMouseEntered) { onClick(); }
    this->value = 0;
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
  juce::AudioProcessorEditor& editor;
  const juce::RangedAudioParameter* const parameter;

  Scale& scale;
  juce::ParameterAttachment attachment;

  void showHostMenuNative(juce::Point<int> position) {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->showNativeMenu(position);
      }
    }
  }

  void showHostMenuJuce() {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->getEquivalentPopupMenu().showMenuAsync(
          juce::PopupMenu::Options().withTargetComponent(this));
      }
    }
  }

  void updateStatusBar() {
    this->statusBar.setText(std::format("{}: {} {}", parameter->getName(256).toRawUTF8(),
                                        this->value >= float(0.5) ? "ON" : "OFF",
                                        parameter->getLabel().toRawUTF8()));
  }

  void setInternalValue(float newValue) {
    if (this->value == newValue) { return; }
    this->value = newValue;
    this->repaint();

    if (this->isMouseEntered || this->hasKeyboardFocus(true)) { updateStatusBar(); }

    if (auto* handler = this->getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void toggleValue() {
    auto newValue = this->value >= float(1) ? float(0) : float(1);
    setInternalValue(newValue);
    attachment.setValueAsCompleteGesture(newValue >= float(1) ? float(scale.getMax())
                                                              : float(scale.getMin()));
  }

public:
  ToggleButton(juce::AudioProcessorEditor& editor, Palette& palette, juce::UndoManager* undoManager,
               juce::RangedAudioParameter* parameter, Scale& scale, StatusBar& statusBar,
               NumberEditor& numberEditor, const juce::String& label, const juce::String& hint)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint), editor(editor),
        parameter(parameter), scale(scale),
        attachment(
          *parameter,
          [&](float newRaw) {
            auto normalized = newRaw >= scale.getMax() ? float(1) : float(0);
            setInternalValue(normalized);
          },
          undoManager) {
    attachment.sendInitialUpdate();
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::toggle, [&]() { toggleValue(); });
    actions.addAction(juce::AccessibilityActionType::showMenu, [&]() { showHostMenuJuce(); });

    return std::make_unique<ButtonAccessibilityHandler>(
      *this, juce::AccessibilityRole::toggleButton, std::move(actions),
      [this](juce::AccessibleState state) {
        state = state.withCheckable();
        if (this->value >= float(1)) { state = state.withChecked(); }
        return state;
      },
      [this]() { return this->label + (this->value >= 0.5f ? " On" : " Off"); });
  }

  void mouseEnter(const juce::MouseEvent& event) override {
    this->ButtonBase<style>::mouseEnter(event);
    updateStatusBar();
  }

  void mouseDown(const juce::MouseEvent& event) override {
    if (event.mods.isRightButtonDown()) {
      showHostMenuNative(editor.getMouseXYRelative());
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
  juce::AudioProcessorEditor& editor;
  const juce::RangedAudioParameter* const parameter;

  Scale& scale;
  juce::ParameterAttachment attachment;

  void showHostMenuNative(juce::Point<int> position) {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->showNativeMenu(position);
      }
    }
  }

  void showHostMenuJuce() {
    if (auto* hostContext = editor.getHostContext()) {
      if (auto hostContextMenu = hostContext->getContextMenuForParameter(parameter)) {
        hostContextMenu->getEquivalentPopupMenu().showMenuAsync(
          juce::PopupMenu::Options().withTargetComponent(this));
      }
    }
  }

  void updateStatusBar() {
    this->statusBar.setText(std::format("{}: {} {}", parameter->getName(256).toRawUTF8(),
                                        this->value >= float(0.5) ? "ON" : "OFF",
                                        parameter->getLabel().toRawUTF8()));
  }

  void setInternalValue(float newValue) {
    if (this->value == newValue) { return; }
    this->value = newValue;
    this->repaint();

    if (this->isMouseEntered || this->hasKeyboardFocus(true)) { updateStatusBar(); }
  }

  void setValue(bool isPressed) {
    auto newValue = isPressed ? float(1) : float(0);
    setInternalValue(newValue);
    attachment.setValueAsCompleteGesture(isPressed ? float(scale.getMax()) : float(scale.getMin()));
  }

public:
  MomentaryButton(juce::AudioProcessorEditor& editor, Palette& palette,
                  juce::UndoManager* undoManager, juce::RangedAudioParameter* parameter,
                  Scale& scale, StatusBar& statusBar, NumberEditor& numberEditor,
                  const juce::String& label, const juce::String& hint)
      : ButtonBase<style>(editor, palette, statusBar, numberEditor, label, hint), editor(editor),
        parameter(parameter), scale(scale),
        attachment(
          *parameter,
          [&](float newRaw) {
            auto normalized = newRaw >= scale.getMax() ? float(1) : float(0);
            setInternalValue(normalized);
          },
          undoManager) {
    attachment.sendInitialUpdate();
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [&]() {
      if (this->value < float(1)) { setValue(true); }
      juce::Timer::callAfterDelay(100, [&]() {
        if (this->value > float(0)) { setValue(false); }
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
      showHostMenuNative(editor.getMouseXYRelative());
      return;
    }
    setValue(true);
  }

  void mouseUp(const juce::MouseEvent&) override {
    if (this->value) { setValue(false); }
  }

  bool keyStateChanged(bool) override {
    using KP = juce::KeyPress;
    if (KP::isKeyCurrentlyDown(KP::returnKey) || KP::isKeyCurrentlyDown(KP::spaceKey)) {
      if (this->value < float(1)) { setValue(true); }
      return true;
    }
    if (this->value > float(0)) {
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
