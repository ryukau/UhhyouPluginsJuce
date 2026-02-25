// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "numbereditor.hpp"
#include "style.hpp"

#include <format>
#include <functional>
#include <numbers>
#include <unordered_set>

namespace Uhhyou {

class ParameterLockRegistry {
private:
  std::unordered_set<const juce::AudioProcessorParameter*> locked;
  std::function<void()> onChange;

public:
  void setOnChange(std::function<void()> callback) { onChange = callback; }

  bool isLocked(const juce::AudioProcessorParameter* p) const { return locked.contains(p); }

  void lock(const juce::AudioProcessorParameter* p) {
    if (p == nullptr) { return; }
    locked.insert(p);
    if (onChange) { onChange(); }
  }

  void toggle(const juce::AudioProcessorParameter* p) {
    if (p == nullptr) { return; }

    if (locked.contains(p)) {
      locked.erase(p);
    } else {
      locked.insert(p);
    }

    if (onChange) { onChange(); }
  }

  const auto& getLockedParameters() const { return locked; }
};

class LockableLabel : public juce::Component, public juce::SettableTooltipClient {
public:
  enum class Orientation { horizontal, vertical };

private:
  class LockableLabelAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    LockableLabelAccessibilityHandler(LockableLabel& labelToWrap,
                                      juce::AccessibilityActions actions)
        : juce::AccessibilityHandler(labelToWrap, juce::AccessibilityRole::toggleButton,
                                     std::move(actions)),
          label(labelToWrap) {}

    juce::AccessibleState getCurrentState() const override {
      auto state = juce::AccessibilityHandler::getCurrentState().withCheckable();
      if (label.parameter && label.locks.isLocked(label.parameter)) { state = state.withChecked(); }
      return state;
    }

    juce::String getTitle() const override {
      juce::String status = (label.parameter && label.locks.isLocked(label.parameter))
        ? ", Randomize off."
        : ", Randomize on.";
      return label.labelText + status;
    }

  private:
    LockableLabel& label;
  };

  ParameterLockRegistry& locks;
  Palette& pal;
  StatusBar& statusBar;
  const juce::AudioProcessorParameter* const parameter;
  juce::String labelText;
  juce::Justification justification;
  Orientation orientation;
  bool isHovering = false;

  void updateStatusBar() {
    statusBar.setText(std::format("{}: Randomization {}", labelText.toRawUTF8(),
                                  locks.isLocked(parameter) ? "OFF" : "ON"));
  }

  void toggleLock() {
    if (!parameter) { return; }
    locks.toggle(parameter);
    updateStatusBar();
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

public:
  LockableLabel(ParameterLockRegistry& locks, Palette& palette, StatusBar& statusBar,
                const juce::String& label, const juce::AudioProcessorParameter* const parameter,
                juce::Justification justification,
                Orientation orientation = Orientation::horizontal)
      : locks(locks), pal(palette), statusBar(statusBar), parameter(parameter), labelText(label),
        justification(justification), orientation(orientation) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::toggle, [this]() { toggleLock(); });
    return std::make_unique<LockableLabelAccessibilityHandler>(*this, std::move(actions));
  }

  void paint(juce::Graphics& ctx) override {
    auto bounds = getLocalBounds().toFloat();
    if (justification.testFlags(juce::Justification::left)) {
      bounds.removeFromLeft(pal.textSizeUi());
    }

    if (orientation == Orientation::vertical) {
      float cx = bounds.getCentreX();
      float cy = bounds.getCentreY();
      constexpr float pi = std::numbers::pi_v<float>;
      ctx.addTransform(juce::AffineTransform::rotation(-pi * float(0.5), cx, cy));

      float h = bounds.getHeight();
      float w = bounds.getWidth();
      bounds.setSize(h, w);
      bounds.setCentre(cx, cy);
    }

    // Draw Text
    ctx.setFont(pal.getFont(pal.textSizeUi()));
    ctx.setColour(locks.isLocked(parameter) ? pal.foregroundInactive() : pal.foreground());
    ctx.drawText(labelText, bounds, justification);
  }

  void mouseEnter(const juce::MouseEvent&) override {
    isHovering = true;
    updateStatusBar();
    repaint();
  }

  void mouseExit(const juce::MouseEvent&) override {
    isHovering = false;
    statusBar.clear();
    repaint();
  }

  void mouseDown(const juce::MouseEvent&) override { toggleLock(); }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      toggleLock();
      return true;
    }
    return false;
  }

  void focusGained(juce::Component::FocusChangeType) override { repaint(); }
  void focusLost(juce::Component::FocusChangeType) override { repaint(); }
};

} // namespace Uhhyou
