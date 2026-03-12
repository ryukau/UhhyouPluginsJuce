// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "../parameter.hpp"
#include "Uhhyou/gui/style.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <limits>

namespace Uhhyou {

class EnvelopeDisplay : public juce::AnimatedAppComponent {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeDisplay)

protected:
  Palette& pal_;
  std::array<std::atomic<float>, nChannel>& meterInput_;
  std::array<std::atomic<float>, nChannel>& meterEnvelope_;

  bool isMouseEntered_ = false;
  juce::Font font_;
  juce::PathStrokeType lineStrokeType_;
  std::array<std::deque<float>, nChannel> inPeak_;
  std::array<std::deque<float>, nChannel> envelope_;

public:
  EnvelopeDisplay(juce::Component& parent, Palette& palette,
                  std::array<std::atomic<float>, nChannel>& meterInput,
                  std::array<std::atomic<float>, nChannel>& meterEnvelope)
      : pal_(palette), meterInput_(meterInput), meterEnvelope_(meterEnvelope),
        font_(palette.getFont(TextSize::normal)),
        lineStrokeType_(palette.borderWidth(), juce::PathStrokeType::JointStyle::curved,
                        juce::PathStrokeType::EndCapStyle::rounded) {
    setSynchroniseToVBlank(true);
    parent.addAndMakeVisible(*this, 0);
  }

  virtual ~EnvelopeDisplay() override {}

  virtual void resized() override {
    font_ = pal_.getFont(TextSize::normal);
    lineStrokeType_.setStrokeThickness(pal_.borderWidth());

    const auto size = size_t(getWidth() - 2 * pal_.borderWidth());
    for (auto& x : inPeak_) { x.resize(size, float(1)); }
    for (auto& x : envelope_) { x.resize(size, float(1)); }
  }

  virtual void update() override {
    for (size_t i = 0; i < inPeak_.size(); ++i) {
      inPeak_[i].push_back(float(1) - meterInput_[i].load());
      inPeak_[i].pop_front();
    }

    for (size_t i = 0; i < envelope_.size(); ++i) {
      envelope_[i].push_back(float(1) - meterEnvelope_[i].load());
      envelope_[i].pop_front();
    }
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal_.borderWidth(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());
    const float lineOffsetX = pal_.borderWidth();

    // Background.
    ctx.setColour(pal_.surface());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Input.
    ctx.setColour(pal_.main());
    for (const auto& pk : inPeak_) {
      if (pk.size() < 2) { continue; }
      juce::Path line;
      line.startNewSubPath(lineOffsetX, height * pk[0]);
      for (int i = 1; i < pk.size(); ++i) { line.lineTo(lineOffsetX + float(i), height * pk[i]); }
      line.lineTo(width, height);
      line.lineTo(0, height);
      line.closeSubPath();
      ctx.fillPath(line);
    }

    // Envelope.
    ctx.setColour(pal_.foreground());
    for (const auto& env : envelope_) {
      if (env.size() < 2) { continue; }
      juce::Path line;
      line.startNewSubPath(lineOffsetX, height * env[0]);
      for (int i = 1; i < env.size(); ++i) { line.lineTo(lineOffsetX + float(i), height * env[i]); }
      ctx.strokePath(line, lineStrokeType_);
    }

    // Border.
    ctx.setColour(isMouseEntered_ ? pal_.main() : pal_.border());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);
  }

  virtual void mouseMove(const juce::MouseEvent&) override {}

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered_ = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered_ = false;
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent&) override {}
  virtual void mouseDrag(const juce::MouseEvent&) override {}
  virtual void mouseUp(const juce::MouseEvent&) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent&) override {}

  virtual void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
};

} // namespace Uhhyou
