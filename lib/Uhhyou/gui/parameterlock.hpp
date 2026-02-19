// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

#include <functional>
#include <numbers>
#include <unordered_set>

namespace Uhhyou {

class ParameterLockRegistry {
private:
  std::unordered_set<const juce::AudioProcessorParameter *> locked;
  std::function<void()> onChange;

public:
  void setOnChange(std::function<void()> callback) { onChange = callback; }

  bool isLocked(const juce::AudioProcessorParameter *p) const
  {
    return locked.contains(p);
  }

  void lock(const juce::AudioProcessorParameter *p)
  {
    if (p == nullptr) return;
    locked.insert(p);
    if (onChange) onChange();
  }

  void toggle(const juce::AudioProcessorParameter *p)
  {
    if (p == nullptr) return;

    if (locked.contains(p)) {
      locked.erase(p);
    } else {
      locked.insert(p);
    }

    if (onChange) onChange();
  }

  const auto &getLockedParameters() const { return locked; }
};

class LockableLabel : public juce::Component {
public:
  enum class Orientation { horizontal, vertical };

private:
  ParameterLockRegistry &locks;
  Palette &pal;
  const juce::AudioProcessorParameter *const parameter;
  juce::String labelText;
  juce::Justification justification;
  Orientation orientation;
  bool isHovering = false;

public:
  LockableLabel(
    ParameterLockRegistry &locks,
    Palette &palette,
    const juce::String &label,
    const juce::AudioProcessorParameter *const parameter,
    juce::Justification justification,
    Orientation orientation = Orientation::horizontal)
    : locks(locks)
    , pal(palette)
    , parameter(parameter)
    , labelText(label)
    , justification(justification)
    , orientation(orientation)
  {
    setWantsKeyboardFocus(true);
  }

  void paint(juce::Graphics &ctx) override
  {
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
    ctx.setColour(
      locks.isLocked(parameter) ? pal.foregroundInactive() : pal.foreground());
    ctx.drawText(labelText, bounds, justification);
  }

  void mouseEnter(const juce::MouseEvent &) override
  {
    isHovering = true;
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    repaint();
  }

  void mouseExit(const juce::MouseEvent &) override
  {
    isHovering = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
  }

  void mouseDown(const juce::MouseEvent &) override
  {
    if (parameter) {
      locks.toggle(parameter);
      repaint();
    }
  }

  bool keyPressed(const juce::KeyPress &key) override
  {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      if (!parameter) return false;
      locks.toggle(parameter);
      repaint();
      return true;
    }
    return false;
  }

  void focusGained(juce::Component::FocusChangeType) override { repaint(); }
  void focusLost(juce::Component::FocusChangeType) override { repaint(); }
};

} // namespace Uhhyou
