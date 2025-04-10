// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cinttypes>
#include <vector>

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
  juce::Justification justification = juce::Justification::centred;

  GroupLabel(const juce::String &text) : text(text), rect({0, 0, 0, 0}) {}

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

struct LabeledWidget {
  uint64_t option;
  const juce::String &label;
  juce::Component &widget;

  enum Layout : decltype(option) { showLabel = 1, expand = 2 };

  LabeledWidget(
    const juce::String &label,
    juce::Component &widget,
    uint64_t option = Layout::showLabel)
    : label(label), widget(widget), option(option)
  {
  }
};

// Return value is the `top` of next section.
inline int layoutVerticalSection(
  std::vector<TextLabel> &labels,
  std::vector<GroupLabel> &groupLabels,
  int left,
  int top,
  int sectionWidth,
  int labelWidth,
  int widgetWidth,
  int labelXIncrement,
  int labelHeight,
  int labelYIncrement,
  const juce::String &groupTitle,
  std::vector<LabeledWidget> data)
{
  using Rect = juce::Rectangle<int>;
  using Opt = LabeledWidget::Layout;

  if (groupTitle.isNotEmpty()) {
    groupLabels.emplace_back(groupTitle, Rect{left, top, sectionWidth, labelHeight});
    top += labelYIncrement;
  }

  const int left1 = left + labelXIncrement;
  for (auto &line : data) {
    if (line.option & (Opt::showLabel + Opt::expand)) {
      labels.emplace_back(line.label, Rect{left, top, labelWidth, labelHeight});
    }

    if (line.option & Opt::expand) {
      line.widget.setBounds(Rect{left, top, sectionWidth, labelHeight});
    } else {
      line.widget.setBounds(Rect{left1, top, widgetWidth, labelHeight});
    }

    top += labelYIncrement;
  }

  return top;
}

inline int layoutActionSection(
  std::vector<GroupLabel> &groupLabels,
  int left,
  int top,
  int sectionWidth,
  int labelWidth,
  int widgetWidth,
  int labelXIncrement,
  int labelHeight,
  int labelYIncrement,
  juce::Component &undoButton,
  juce::Component &redoButton,
  juce::Component &randomizeButton,
  juce::Component &presetManager)
{
  using Rect = juce::Rectangle<int>;

  groupLabels.emplace_back("Action", Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  undoButton.setBounds(Rect{left, top, labelWidth, labelHeight});
  redoButton.setBounds(Rect{left + labelXIncrement, top, widgetWidth, labelHeight});

  top += labelYIncrement;
  randomizeButton.setBounds(Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  groupLabels.emplace_back("Preset", Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  presetManager.setBounds(Rect{left, top, sectionWidth, labelHeight});

  return top + labelYIncrement;
}

inline void setDefaultColor(juce::LookAndFeel_V4 &laf, Palette &pal)
{
  laf.setColour(juce::CaretComponent::caretColourId, pal.foreground());

  laf.setColour(juce::TextEditor::backgroundColourId, pal.background());
  laf.setColour(juce::TextEditor::textColourId, pal.foreground());
  laf.setColour(juce::TextEditor::highlightColourId, pal.overlayHighlight());
  laf.setColour(juce::TextEditor::highlightedTextColourId, pal.foreground());
  laf.setColour(juce::TextEditor::outlineColourId, pal.border());
  laf.setColour(juce::TextEditor::focusedOutlineColourId, pal.highlightMain());

  juce::LookAndFeel::setDefaultLookAndFeel(&laf);
}

} // namespace Uhhyou
