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
#include <deque>
#include <limits>
#include <numbers>

namespace Uhhyou {

class LfoPhaseDisplay : public juce::AnimatedAppComponent {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoPhaseDisplay)

  static constexpr size_t trailLength = 64; // frames.

  Palette &pal;
  std::array<std::atomic<float>, nChannel> &phase;

  juce::Font font;
  juce::PathStrokeType stroke;
  std::array<std::deque<float>, nChannel> trails;

public:
  LfoPhaseDisplay(
    Component &parent,
    Palette &palette,
    std::array<std::atomic<float>, nChannel> &lfoPhase)
    : pal(palette)
    , phase(lfoPhase)
    , font(palette.getFont(palette.textSizeSmall()))
    , stroke(
        4 * palette.borderThin(),
        juce::PathStrokeType::JointStyle::curved,
        juce::PathStrokeType::EndCapStyle::rounded)

  {
    setSynchroniseToVBlank(true);
    parent.addAndMakeVisible(*this, 0);

    for (auto &x : trails) x.resize(trailLength, float(0));
  }

  virtual ~LfoPhaseDisplay() override {}

  virtual void resized() override
  {
    font = pal.getFont(pal.textSizeSmall());
    stroke.setStrokeThickness(4 * pal.borderThin());
  }

  virtual void update() override
  {
    for (size_t i = 0; i < trails.size(); ++i) {
      trails[i].push_back(phase[i].load(std::memory_order_relaxed));
      trails[i].pop_front();
    }
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    constexpr float twopi = float(2) * std::numbers::pi_v<float>;
    constexpr float halfpi = float(0.5) * std::numbers::pi_v<float>;

    const float lw1 = pal.borderThin();
    const float w = float(getWidth());
    const float h = float(getHeight());

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillRect(float(0), float(0), w, h);

    // Geometry.
    const float cx = w * float(0.5);
    const float cy = h * float(0.5);
    const float offset = stroke.getStrokeThickness();
    const float radius = (std::min(w, h) * float(0.5)) - (float(2) * offset);

    // Circle guide.
    ctx.setColour(pal.border().withAlpha(float(0.2)));
    const float diameter = float(2) * radius;
    ctx.drawEllipse(cx - radius, cy - radius, diameter, diameter, lw1);

    // Labels.
    const std::array colors{pal.foreground(), pal.highlightButton()};
    {
      const float side = font.getHeight();
      const float y = h - side;
      ctx.setFont(font);

      juce::Rectangle<float> labelBoundsL{float(0), y, side, side};
      ctx.setColour(colors[0]);
      ctx.drawText("L", labelBoundsL, juce::Justification::centred);

      juce::Rectangle<float> labelBoundsR{side, y, side, side};
      ctx.setColour(colors[1]);
      ctx.drawText("R", labelBoundsR, juce::Justification::centred);
    }

    // Phase trails.
    for (size_t ch = trails.size() - 1; ch < trails.size(); --ch) {
      const auto &trail = trails[ch];
      if (trail.empty()) continue;

      const auto color = colors[ch % colors.size()];
      auto strk = stroke;
      for (size_t i = 1; i < trail.size(); ++i) {
        const float p0 = trail[i - 1];
        const float diff = std::remainder(trail[i] - p0, float(1));
        const float p1 = p0 + diff;

        const float alpha = float(i) / float(trail.size() - 1);
        ctx.setColour(color.withAlpha(alpha / 2));
        strk.setStrokeThickness(offset * alpha);

        juce::Path path;
        path.addCentredArc(cx, cy, radius, radius, 0, p0 * twopi, p1 * twopi, true);
        ctx.strokePath(path, strk);
      }

      // Draw the head (latest value).
      ctx.setColour(color.withAlpha(float(0.5)));
      const float headPhase = trail.back();
      const float headAngle = (headPhase * twopi) - halfpi;

      const float hx = cx + std::cos(headAngle) * radius;
      const float hy = cy + std::sin(headAngle) * radius;

      const float dotSize = float(2) * offset;
      ctx.fillEllipse(hx - offset, hy - offset, dotSize, dotSize);
      ctx.drawLine(cx, cy, hx, hy);
    }
  }
};

} // namespace Uhhyou
