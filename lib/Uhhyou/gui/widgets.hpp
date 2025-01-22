// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "barbox.hpp"
#include "button.hpp"
#include "buttonarray.hpp"
#include "combobox.hpp"
#include "knob.hpp"
#include "popupview.hpp"
#include "presetmanager.hpp"
#include "style.hpp"
#include "tabview.hpp"

namespace Uhhyou {

class TestTile : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestTile)

  juce::Colour color;
  juce::uint8 alpha = 0xff;

public:
  TestTile(juce::Colour color, juce::uint8 alpha) : color(color), alpha(alpha) {}

  virtual void paint(juce::Graphics &ctx) override
  {
    ctx.setColour(color);
    ctx.fillAll();
  }
};

struct Line {
  juce::Point<float> start{};
  juce::Point<float> end{};
  float thickness{};

  Line(juce::Point<float> start, juce::Point<float> end, float thickness)
    : start(start), end(end), thickness(thickness)
  {
  }

  void paint(juce::Graphics &ctx) const
  {
    ctx.drawLine(start.x, start.y, end.x, end.y, thickness);
  }
};

struct TextLabel {
  juce::String text;
  juce::Rectangle<int> rect;
  juce::Justification justification;

  TextLabel(
    const juce::String &text,
    const juce::Rectangle<int> &rect,
    juce::Justification justification = juce::Justification::centred)
    : text(text), rect(rect), justification(justification)
  {
  }

  void paint(juce::Graphics &ctx) const { ctx.drawText(text, rect, justification); }
};

struct GroupLabel {
  juce::String text;
  juce::Rectangle<int> rect;
  juce::Justification justification;

  GroupLabel(
    const juce::String &text,
    const juce::Rectangle<int> &rect,
    juce::Justification justification = juce::Justification::centred)
    : text(text), rect(rect), justification(justification)
  {
  }

  void
  paint(juce::Graphics &ctx, juce::Font &font, float lineWidth, float marginWidth) const
  {
    ctx.drawText(text, rect, justification);

    auto textWidth = juce::GlyphArrangement::getStringWidth(font, text);

    auto lineY = rect.getY() + float(0.5) * (rect.getHeight() - lineWidth);
    auto centerX = float(0.5) * rect.getWidth();
    auto offsetFronCenter = marginWidth + float(0.5) * textWidth;
    auto rectWidth = centerX - offsetFronCenter;
    ctx.fillRoundedRectangle(
      float(rect.getX()), lineY, rectWidth, lineWidth, lineWidth / 2);
    ctx.fillRoundedRectangle(
      rect.getX() + centerX + offsetFronCenter, lineY, rectWidth, lineWidth,
      lineWidth / 2);
  }
};

} // namespace Uhhyou
