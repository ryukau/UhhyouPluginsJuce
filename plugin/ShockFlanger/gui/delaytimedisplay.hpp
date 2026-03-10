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
#include <format>
#include <limits>
#include <numbers>

namespace Uhhyou {

class DelayTimeDisplay : public juce::AnimatedAppComponent {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayTimeDisplay)

  static constexpr float historyMs = float(2000);
  static constexpr float fadeDurationMs = float(500);

  Palette& pal;
  ValueReceivers& pv;

  juce::Font fontLabel;
  juce::Font fontSmall;

  struct TracePoint {
    float value;
    float ms; // Time duration this sample represents.
  };

  using TraceDeque = std::deque<TracePoint>;
  std::array<std::array<TraceDeque, 2>, nChannel> trailUpper;
  std::array<std::array<TraceDeque, 2>, nChannel> trailLower;

  // [Channel][Lane][0=Min, 1=Max]
  std::array<std::array<std::array<float, 2>, 2>, nChannel> limitWarnings{};

public:
  DelayTimeDisplay(Component& parent, Palette& palette, ValueReceivers& valueReceiver)
      : pal(palette), pv(valueReceiver), fontLabel(palette.getFont(TextSize::normal)),
        fontSmall(palette.getFont(TextSize::small)) {
    setSynchroniseToVBlank(true);
    parent.addAndMakeVisible(*this, 0);
  }

  virtual ~DelayTimeDisplay() override {}

  virtual void resized() override {
    fontLabel = pal.getFont(TextSize::normal);
    fontSmall = pal.getFont(TextSize::small);
  }

  virtual void update() override {
    // Clamp dt to reasonable bounds to prevent instability.
    const float dt
      = std::clamp(static_cast<float>(getMillisecondsSinceLastUpdate()), float(1), float(100));
    const float decay = dt / fadeDurationMs;

    auto updateLane = [&](TraceDeque& trail, const std::atomic<float>& source) {
      trail.push_back({source.load(std::memory_order_relaxed), dt});

      // Lazy pruning: only check if size gets large.
      if (trail.size() > 20) {
        float total = float(0);
        for (const auto& pt : trail) { total += pt.ms; }

        while (total > historyMs && !trail.empty()) {
          total -= trail.front().ms;
          trail.pop_front();
        }
      }
    };

    for (size_t i = 0; i < nChannel; ++i) {
      for (size_t j = 0; j < 2; ++j) {
        updateLane(trailUpper[i][j], pv.displayDelayTimeUpper[i][j]);
        updateLane(trailLower[i][j], pv.displayDelayTimeLower[i][j]);

        auto& warnings = limitWarnings[i][j];

        warnings[0] = std::max(float(0), warnings[0] - decay);
        warnings[1] = std::max(float(0), warnings[1] - decay);

        if (!trailLower[i][j].empty()) {
          if (trailLower[i][j].back().value <= float(0)) { warnings[0] = float(1); }
        }
        if (!trailUpper[i][j].empty()) {
          if (trailUpper[i][j].back().value >= float(10)) { warnings[1] = float(1); }
        }
      }
    }
  }

  virtual void paint(juce::Graphics& ctx) override {
    const auto fullBounds = getLocalBounds().toFloat();
    const float headerHeight = fontSmall.getHeight();
    const float sideLabelWidth = 3 * fontSmall.getHeight();

    auto remainingBounds = fullBounds;
    auto headerBounds = remainingBounds.removeFromTop(headerHeight);
    auto sideLabelBounds = remainingBounds.removeFromLeft(sideLabelWidth);
    auto graphBounds = remainingBounds;

    // Background.
    ctx.setColour(pal.background());
    ctx.fillAll();

    // Grid & Ticks.
    constexpr std::array<float, 6> tickMs{
      float(0.1), float(1), float(10), float(100), float(1000), float(10000),
    };
    constexpr float minLog10 = float(-1);
    constexpr float maxLog10 = float(4);
    constexpr float logRange = maxLog10 - minLog10;

    auto timeToGraphX = [&](float seconds) -> float {
      const auto s = std::max(seconds, float(0.00001));
      const auto ms = float(1000) * s;
      const auto logVal = std::log10(ms);
      const auto normalized = (logVal - minLog10) / logRange;
      return graphBounds.getX()
        + graphBounds.getWidth() * std::clamp(normalized, float(0), float(1));
    };

    ctx.setFont(fontSmall);
    ctx.setColour(pal.border());

    const float gridLineWidth = float(0.125) * pal.borderWidth();
    for (size_t i = 0; i < tickMs.size(); ++i) {
      float ms = tickMs[i];
      float x = timeToGraphX(ms / float(1000));

      // Grid line.
      ctx.setColour(pal.border());
      ctx.drawLine(x, headerBounds.getBottom(), x, fullBounds.getBottom(), gridLineWidth);

      // Label.
      juce::String labelText = std::format("{}", ms);
      const float margin = float(4) * pal.borderWidth();
      const float textW = juce::GlyphArrangement::getStringWidth(fontSmall, labelText);
      juce::Rectangle<float> labelRect(0, 0, textW + margin, headerBounds.getHeight());

      if (i == tickMs.size() - 1 || ms >= float(10)) {
        // Last tick is right aligned to stay in bounds.
        labelRect.setRight(x - 2 * pal.borderWidth());
        ctx.drawText(labelText, labelRect.toNearestInt(), juce::Justification::centredRight, false);
      } else {
        labelRect.setCentre(x, headerBounds.getCentreY());
        ctx.drawText(labelText, labelRect.toNearestInt(), juce::Justification::centred, false);
      }
    }

    // Side labels.
    std::array<std::array<juce::String, 2>, nChannel> labelNames{{{"L0", "L1"}, {"R0", "R1"}}};
    const float laneHeight = graphBounds.getHeight() / float(nChannel * 2);

    ctx.setColour(pal.foreground());
    auto currentSideBounds = sideLabelBounds;
    for (size_t i = 0; i < nChannel; ++i) {
      for (size_t j = 0; j < 2; ++j) {
        auto area = currentSideBounds.removeFromTop(laneHeight);
        ctx.drawText(labelNames[i][j], area.toNearestInt(), juce::Justification::centred);
      }
    }

    // Waterfall graph.
    auto currentGraphBounds = graphBounds;
    auto minTrailWidth = pal.borderWidth();
    auto minReticleWidth = pal.borderWidth();
    for (size_t i = 0; i < nChannel; ++i) {
      for (size_t j = 0; j < 2; ++j) {
        auto laneArea = currentGraphBounds.removeFromTop(laneHeight);

        ctx.setColour(pal.border());
        ctx.drawLine(laneArea.getX(), laneArea.getY(), fullBounds.getRight(), laneArea.getY(),
                     gridLineWidth);

        juce::Graphics::ScopedSaveState stateSaver(ctx);
        ctx.reduceClipRegion(laneArea.toNearestInt());

        const auto& tLower = trailLower[i][j];
        const auto& tUpper = trailUpper[i][j];

        float currentY = laneArea.getBottom();
        float accumMs = float(0);

        // Draw from Bottom (Newest) to Top (Oldest)
        size_t count = std::min(tLower.size(), tUpper.size());

        for (size_t k = 0; k < count; ++k) {
          size_t idx = count - 1 - k;
          const auto& ptL = tLower[idx];
          const auto& ptR = tUpper[idx];

          float segmentHeight = (ptL.ms / historyMs) * laneArea.getHeight();
          float yBottom = currentY;
          float yTop = yBottom - segmentHeight;

          float screenX1 = timeToGraphX(ptL.value);
          float screenX2 = timeToGraphX(ptR.value);

          float rawWidth = screenX2 - screenX1;
          float drawWidth = std::max(minTrailWidth, rawWidth);

          float progress = accumMs / historyMs;
          float alpha = std::clamp(float(1) - progress * progress, float(0), float(1));

          ctx.setColour(pal.main().withAlpha(alpha));
          ctx.fillRect(screenX1, yTop, drawWidth, yBottom - yTop + float(0.5) * minTrailWidth);

          currentY = yTop;
          accumMs += ptL.ms;

          if (currentY < laneArea.getY()) { break; }
        }

        // Current value reticle.
        if (!tLower.empty()) {
          float curL = tLower.back().value;
          float curR = tUpper.back().value;

          float sx1 = timeToGraphX(curL);
          float sx2 = timeToGraphX(curR);

          float rawWidth = sx2 - sx1;
          float trailVisualWidth = std::max(minTrailWidth, rawWidth);
          float reticleWidth = std::max(minReticleWidth, rawWidth);

          float reticleX = sx1 + (trailVisualWidth - reticleWidth) * float(0.5);
          float cy = laneArea.getBottom() - minTrailWidth;

          ctx.setColour(pal.foreground());
          ctx.fillRect(reticleX, cy - minReticleWidth, reticleWidth, minReticleWidth);
        }

        // Limit warnings (Fading).
        float alphaMin = limitWarnings[i][j][0];
        float alphaMax = limitWarnings[i][j][1];
        if (alphaMin > float(0) || alphaMax > float(0)) {
          const float indH = laneArea.getHeight();
          const float indW = 3 * pal.borderWidth();
          const float indY = laneArea.getBottom() - indH;
          if (alphaMin > float(0)) {
            ctx.setColour(pal.warning().withAlpha(alphaMin));
            ctx.fillRect(laneArea.getX(), indY, indW, indH);
          }
          if (alphaMax > float(0)) {
            ctx.setColour(pal.warning().withAlpha(alphaMax));
            ctx.fillRect(laneArea.getRight() - indW, indY, indW, indH);
          }
        }
      }
    }

    ctx.setColour(pal.border());
    const float bottomLineY = fullBounds.getBottom() - float(0.5) * gridLineWidth;
    ctx.drawLine(currentGraphBounds.getX(), bottomLineY, fullBounds.getRight(), bottomLineY,
                 gridLineWidth);
  }
};

} // namespace Uhhyou
