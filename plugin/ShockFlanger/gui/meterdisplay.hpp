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
#include <atomic>
#include <cmath>
#include <format>
#include <limits>
#include <numbers>

namespace Uhhyou {

class MeterDisplay : public juce::AnimatedAppComponent {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterDisplay)

  Palette& pal_;
  std::array<std::atomic<float>, nChannel>& peakAmp_;

  struct MeterState {
    float peakN{}; // N: normalized in [0, 1].
    float holdN{}; // N: normalized in [0, 1].
    float holdDecibel = -std::numeric_limits<float>::infinity();
    int holdTimer{};
  };
  std::array<MeterState, nChannel> meter_{};

  juce::String labelTitle_;
  juce::Font fontLabel_;
  juce::Font fontSmall_;
  juce::PathStrokeType stroke_;

  static constexpr float minDecibel = float(-80);
  static constexpr float maxDecibel = float(+80);
  static constexpr float tickStepDecibel = float(20);

  inline float decibelToNormalized(float db) {
    if (db <= minDecibel) { return float(0); }
    if (db >= maxDecibel) { return float(1); }
    return (db - minDecibel) / (maxDecibel - minDecibel);
  }

  inline float decibelToHeight(float db, float height) const {
    if (db <= minDecibel) { return height; }
    if (db >= maxDecibel) { return float(0); }
    float normalized = (db - minDecibel) / (maxDecibel - minDecibel);
    return height * (float(1) - normalized);
  }

public:
  MeterDisplay(Component& parent, Palette& palette,
               std::array<std::atomic<float>, nChannel>& peakAmplitude, const juce::String& label)
      : pal_(palette), peakAmp_(peakAmplitude), labelTitle_(label),
        fontLabel_(palette.getFont(TextSize::normal)), fontSmall_(palette.getFont(TextSize::small)),
        stroke_(palette.borderWidth(), juce::PathStrokeType::JointStyle::curved,
                juce::PathStrokeType::EndCapStyle::rounded) {
    setSynchroniseToVBlank(true);
    parent.addAndMakeVisible(*this, 0);
  }

  virtual ~MeterDisplay() override {}

  virtual void resized() override {
    fontLabel_ = pal_.getFont(TextSize::normal);
    fontSmall_ = pal_.getFont(TextSize::small);
    stroke_.setStrokeThickness(pal_.borderWidth());
  }

  virtual void update() override {
    constexpr int holdMilliseconds = 1000;

    // `*Tau` is time in seconds to reach 37% of input (x*e^-1).
    constexpr float peakTau = float(0.2);
    constexpr float holdTau = float(0.2);

    const int elapsed = getMillisecondsSinceLastUpdate();
    const float dt = float(0.001) * elapsed;
    const float peakDecay = (dt > float(0)) ? std::exp(-dt / peakTau) : float(1);
    const float holdDecay = (dt > float(0)) ? std::exp(-dt / holdTau) : float(1);

    auto updateHold = [&](size_t i, float db) {
      if (meter_[i].holdN < meter_[i].peakN) {
        meter_[i].holdN = meter_[i].peakN;
        meter_[i].holdDecibel = db;
        meter_[i].holdTimer = 0;
        return;
      }
      if (meter_[i].holdTimer >= holdMilliseconds) {
        meter_[i].holdN *= holdDecay;
        return;
      }
      meter_[i].holdTimer += elapsed;
      if (meter_[i].holdTimer >= holdMilliseconds) {
        const float excess = float(0.001) * (meter_[i].holdTimer - holdMilliseconds);
        meter_[i].holdN *= std::exp(-excess / holdTau);
      }
    };

    auto flushToZero = [](float& value) {
      if (value < std::numeric_limits<float>::epsilon()) { value = float(0); }
    };

    for (size_t i = 0; i < nChannel; ++i) {
      const float amplitude = peakAmp_[i].exchange(float(0), std::memory_order_relaxed);
      const float db = ScaleTools::ampToDB(amplitude);

      meter_[i].peakN = std::max(decibelToNormalized(db), meter_[i].peakN * peakDecay);
      flushToZero(meter_[i].peakN);

      updateHold(i, db);
      flushToZero(meter_[i].holdN);
      if (meter_[i].holdN <= float(0)) {
        meter_[i].holdDecibel = -std::numeric_limits<float>::infinity();
      }
    }
  }

  virtual void paint(juce::Graphics& ctx) override {
    const auto fullBounds = getLocalBounds().toFloat();
    auto bounds = fullBounds;

    const float tickEm = fontSmall_.getHeight();
    const float tickWidth = juce::GlyphArrangement::getStringWidth(fontSmall_, " 80 ");

    // Background.
    ctx.setColour(pal_.background());
    ctx.fillAll();

    // Label.
    auto labelBounds = bounds.removeFromTop(fontLabel_.getHeight() * float(5.0 / 3.0));
    labelBounds.removeFromLeft(tickWidth);
    ctx.setColour(pal_.foreground());
    ctx.setFont(fontLabel_);
    ctx.drawText(labelTitle_, labelBounds.toNearestInt(), juce::Justification::centred);

    // Numeric display.
    auto numberBounds = bounds.removeFromTop(tickEm);
    numberBounds.removeFromLeft(tickWidth);

    const float channelWidth = numberBounds.getWidth() / float(nChannel);
    ctx.setColour(pal_.foreground());
    ctx.setFont(fontSmall_);
    for (size_t i = 0; i < nChannel; ++i) {
      auto area = numberBounds.removeFromLeft(channelWidth);
      ctx.drawText(std::format("{:+3.1f} dB", meter_[i].holdDecibel), area.toNearestInt(),
                   juce::Justification::centred);
    }

    // Ticks.
    bounds.removeFromBottom(float(0.5) * tickEm);
    const auto tickBounds = bounds.removeFromLeft(tickWidth);
    ctx.setColour(pal_.border());

    for (float db = minDecibel; db <= maxDecibel; db += tickStepDecibel) {
      const float bottom = tickBounds.getY() + decibelToHeight(db, tickBounds.getHeight());

      ctx.drawLine(tickBounds.getRight(), bottom, fullBounds.getWidth(), bottom,
                   float(0.25) * pal_.borderWidth());

      // if (db <= minDecibel) continue;

      ctx.drawText(
        juce::String(std::abs(int(db))),
        juce::Rectangle<float>{0, bottom - float(0.5) * tickEm, tickBounds.getWidth(), tickEm}
          .toNearestInt(),
        juce::Justification::centred);
    }

    // Meter.
    auto meterBounds = bounds;

    juce::ColourGradient gradient{pal_.main(), meterBounds.getBottomLeft(), pal_.warning(),
                                  meterBounds.getTopLeft(), false};
    gradient.addColour(std::nextafter(float(0.5), 0.0), pal_.main());
    gradient.addColour(float(0.5), pal_.warning());
    ctx.setGradientFill(gradient);

    auto channelArea = meterBounds;
    for (size_t i = 0; i < nChannel; ++i) {
      auto barRect = channelArea.removeFromLeft(channelWidth);
      barRect.reduce(pal_.borderWidth(), 0);
      auto filledPart = barRect.removeFromBottom(barRect.getHeight() * meter_[i].peakN);
      ctx.fillRect(filledPart);
    }

    float holdL = meterBounds.getX();
    for (size_t i = 0; i < nChannel; ++i) {
      const float holdR = holdL + channelWidth;
      const float& holdN = meter_[i].holdN;
      const float holdH = meterBounds.getY() + meterBounds.getHeight() * (float(1) - holdN)
        + float(0.5) * pal_.borderWidth();
      const float alpha = std::min(float(1), holdN * meterBounds.getHeight());
      ctx.setColour(gradient.getColourAtPosition(holdN).withAlpha(alpha));
      ctx.drawLine(holdL, holdH, holdR, holdH, pal_.borderWidth());
      holdL = holdR;
    }
  }
};

} // namespace Uhhyou
