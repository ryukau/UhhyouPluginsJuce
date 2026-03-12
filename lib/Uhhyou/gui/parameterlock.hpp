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
  std::unordered_set<const juce::AudioProcessorParameter*> locked_;
  std::function<void()> onChange_;

public:
  void setOnChange(std::function<void()> callback) { onChange_ = callback; }

  bool isLocked(const juce::AudioProcessorParameter* p) const { return locked_.contains(p); }

  void lock(const juce::AudioProcessorParameter* p) {
    if (p == nullptr) { return; }
    locked_.insert(p);
    if (onChange_) { onChange_(); }
  }

  void toggle(const juce::AudioProcessorParameter* p) {
    if (p == nullptr) { return; }

    if (locked_.contains(p)) {
      locked_.erase(p);
    } else {
      locked_.insert(p);
    }

    if (onChange_) { onChange_(); }
  }

  const auto& getLockedParameters() const { return locked_; }
};

class LockableLabel : public juce::Component, public juce::SettableTooltipClient {
public:
  enum class Orientation { horizontal, vertical };

private:
  class LockableLabelAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    LockableLabelAccessibilityHandler(LockableLabel& lbl, juce::AccessibilityActions act)
        : juce::AccessibilityHandler(lbl, juce::AccessibilityRole::toggleButton, std::move(act)),
          label_(lbl) {}

    juce::AccessibleState getCurrentState() const override {
      auto state = juce::AccessibilityHandler::getCurrentState().withCheckable();
      if (label_.parameter_ && label_.locks_.isLocked(label_.parameter_)) {
        state = state.withChecked();
      }
      return state;
    }

    juce::String getTitle() const override {
      juce::String status = (label_.parameter_ && label_.locks_.isLocked(label_.parameter_))
        ? ", Randomize off."
        : ", Randomize on.";
      return label_.labelText_ + status;
    }

  private:
    LockableLabel& label_;
  };

  ParameterLockRegistry& locks_;
  Palette& pal_;
  StatusBar& statusBar_;
  const juce::RangedAudioParameter* const parameter_;
  juce::String labelText_;
  juce::Justification justification_;
  Orientation orientation_;
  bool isHovering_ = false;

  void updateStatusBar() {
    statusBar_.setText(std::format("{}: Randomize {}", parameter_->getName(256).toRawUTF8(),
                                   locks_.isLocked(parameter_) ? "OFF" : "ON"));
  }

  void toggleLock() {
    if (!parameter_) { return; }
    locks_.toggle(parameter_);
    updateStatusBar();
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

public:
  LockableLabel(ParameterLockRegistry& locks, Palette& palette, StatusBar& statusBar,
                const juce::String& label, const juce::RangedAudioParameter* const parameter,
                juce::Justification justification,
                Orientation orientation = Orientation::horizontal)
      : locks_(locks), pal_(palette), statusBar_(statusBar), parameter_(parameter),
        labelText_(label), justification_(justification), orientation_(orientation) {
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
    if (justification_.testFlags(juce::Justification::left)) {
      bounds.removeFromLeft(std::floor(pal_.getFontHeight(TextSize::normal)));
    }

    if (orientation_ == Orientation::vertical) {
      float cx = bounds.getCentreX();
      float cy = bounds.getCentreY();
      constexpr float pi = std::numbers::pi_v<float>;
      ctx.addTransform(juce::AffineTransform::rotation(-pi * float(0.5), cx, cy));

      float h = bounds.getHeight();
      float w = bounds.getWidth();
      bounds.setSize(std::floor(h), std::floor(w));
      bounds.setCentre(std::floor(cx), std::floor(cy));
    }

    // Draw Text
    ctx.setFont(pal_.getFont(TextSize::normal));
    ctx.setColour(locks_.isLocked(parameter_) ? pal_.foreground().withAlpha(0.5f)
                                              : pal_.foreground());
    ctx.drawText(labelText_, bounds, justification_);
  }

  void mouseEnter(const juce::MouseEvent&) override {
    isHovering_ = true;
    updateStatusBar();
    repaint();
  }

  void mouseExit(const juce::MouseEvent&) override {
    isHovering_ = false;
    statusBar_.clear();
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
