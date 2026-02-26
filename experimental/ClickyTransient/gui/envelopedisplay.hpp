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
  Palette& pal;
  std::array<std::atomic<float>, nChannel>& meterInput;
  std::array<std::atomic<float>, nChannel>& meterEnvelope;

  bool isMouseEntered = false;
  juce::Font font;
  juce::PathStrokeType lineStrokeType;
  std::array<std::deque<float>, nChannel> inPeak;
  std::array<std::deque<float>, nChannel> envelope;

public:
  EnvelopeDisplay(juce::Component& parent, Palette& palette,
                  std::array<std::atomic<float>, nChannel>& meterInput,
                  std::array<std::atomic<float>, nChannel>& meterEnvelope)
      : pal(palette), meterInput(meterInput), meterEnvelope(meterEnvelope),
        font(palette.getFont(palette.textSizeUi())),
        lineStrokeType(palette.borderThin(), juce::PathStrokeType::JointStyle::curved,
                       juce::PathStrokeType::EndCapStyle::rounded) {
    setSynchroniseToVBlank(true);
    parent.addAndMakeVisible(*this, 0);
  }

  virtual ~EnvelopeDisplay() override {}

  virtual void resized() override {
    font = pal.getFont(pal.textSizeUi());
    lineStrokeType.setStrokeThickness(pal.borderThin());

    const auto size = size_t(getWidth() - 2 * pal.borderThin());
    for (auto& x : inPeak) { x.resize(size, float(1)); }
    for (auto& x : envelope) { x.resize(size, float(1)); }
  }

  virtual void update() override {
    for (size_t i = 0; i < inPeak.size(); ++i) {
      inPeak[i].push_back(float(1) - meterInput[i].load());
      inPeak[i].pop_front();
    }

    for (size_t i = 0; i < envelope.size(); ++i) {
      envelope[i].push_back(float(1) - meterEnvelope[i].load());
      envelope[i].pop_front();
    }
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());
    const float lineOffsetX = pal.borderThin();

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Input.
    ctx.setColour(pal.highlightMain());
    for (const auto& pk : inPeak) {
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
    ctx.setColour(pal.foreground());
    for (const auto& env : envelope) {
      if (env.size() < 2) { continue; }
      juce::Path line;
      line.startNewSubPath(lineOffsetX, height * env[0]);
      for (int i = 1; i < env.size(); ++i) { line.lineTo(lineOffsetX + float(i), height * env[i]); }
      ctx.strokePath(line, lineStrokeType);
    }

    // Border.
    ctx.setColour(isMouseEntered ? pal.highlightAccent() : pal.border());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);
  }

  virtual void mouseMove(const juce::MouseEvent&) override {}

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent&) override {}
  virtual void mouseDrag(const juce::MouseEvent&) override {}
  virtual void mouseUp(const juce::MouseEvent&) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent&) override {}

  virtual void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
};

} // namespace Uhhyou
