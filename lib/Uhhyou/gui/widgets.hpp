// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cinttypes>
#include <functional>
#include <memory>
#include <vector>

#include "parameterlock.hpp"

#include "barbox.hpp"
#include "button.hpp"
#include "buttonarray.hpp"
#include "combobox.hpp"
#include "horizontaldrawer.hpp"
#include "knob.hpp"
#include "lookandfeel.hpp"
#include "navigation.hpp"
#include "popupview.hpp"
#include "presetmanager.hpp"
#include "style.hpp"
#include "tabview.hpp"
#include "xypad.hpp"

namespace Uhhyou {

class GroupLabel {
private:
  juce::String text_;
  juce::Rectangle<int> rect_;
  juce::Justification justification_ = juce::Justification::centred;

public:
  GroupLabel(const juce::String& text) : text_(text), rect_({0, 0, 0, 0}) {}

  GroupLabel(const juce::String& text, const juce::Rectangle<int>& rect,
             juce::Justification justification = juce::Justification::centred)
      : text_(text), rect_(rect), justification_(justification) {}

  void paint(juce::Graphics& ctx, juce::Font& font, float lineWidth, float marginWidth) const {
    juce::Graphics::ScopedSaveState saveState(ctx);
    ctx.addTransform(juce::AffineTransform::translation(float(rect_.getX()), float(rect_.getY())));
    auto localBounds = rect_.withZeroOrigin().toFloat();

    // Text.
    ctx.drawText(text_, localBounds, justification_);

    // Decoration.
    auto textWidth = juce::GlyphArrangement::getStringWidth(font, text_);
    auto centerY = localBounds.getCentreY();
    auto centerX = localBounds.getCentreX();
    auto offsetFromCenter = marginWidth + float(0.5) * textWidth;
    auto lineLength = centerX - offsetFromCenter;

    juce::Path p;
    p.startNewSubPath(6 * lineWidth, centerY);
    p.lineTo(lineLength, centerY);

    auto diamondSize = lineWidth;
    p.startNewSubPath(0, centerY);
    p.lineTo(diamondSize, centerY + diamondSize);
    p.lineTo(2 * diamondSize, centerY);
    p.lineTo(diamondSize, centerY - diamondSize);
    p.closeSubPath();

    p.addPath(p, juce::AffineTransform::scale(float(-1), float(1), centerX, centerY));

    juce::PathStrokeType stroke(lineWidth, juce::PathStrokeType::JointStyle::mitered,
                                juce::PathStrokeType::EndCapStyle::rounded);
    ctx.strokePath(p, stroke);
    ctx.fillPath(p);

    if (text_.isEmpty()) {
      const auto armLength = localBounds.getHeight() / 4.0f;
      const auto diaSize = armLength - lineWidth;

      juce::Path q;

      const auto cy1 = centerY - armLength;
      q.startNewSubPath(centerX, cy1 - diaSize);
      q.lineTo(centerX - diaSize, cy1);
      q.lineTo(centerX, cy1 + diaSize);
      q.lineTo(centerX + diaSize, cy1);
      q.closeSubPath();

      const auto cx1 = centerX - armLength;
      q.startNewSubPath(cx1, centerY - diaSize);
      q.lineTo(cx1 - diaSize, centerY);
      q.lineTo(cx1, centerY + diaSize);
      q.lineTo(cx1 + diaSize, centerY);
      q.closeSubPath();

      const auto cx2 = centerX + armLength;
      q.startNewSubPath(cx2, centerY - diaSize);
      q.lineTo(cx2 - diaSize, centerY);
      q.lineTo(cx2, centerY + diaSize);
      q.lineTo(cx2 + diaSize, centerY);
      q.closeSubPath();

      ctx.fillPath(q);
    }
  }
};

class LabeledWidget : public juce::Component {
public:
  enum Layout { showLabel = 0, expand = 1 };

private:
  Palette& pal_;
  Layout option_;
  juce::Component& widget_;
  LockableLabel lockLabel_;

public:
  LabeledWidget(ParameterLockRegistry& locks, Palette& palette, StatusBar& statusBar,
                const juce::String& label, juce::Component& widget,
                const juce::RangedAudioParameter* const parameter,
                Layout option = Layout::showLabel)
      : pal_(palette), option_(option), widget_(widget),
        lockLabel_(locks, palette, statusBar, label, parameter, juce::Justification::centredLeft,
                   LockableLabel::Orientation::horizontal) {
    if (option_ == Layout::showLabel) {
      addAndMakeVisible(lockLabel_);
    } else {
      locks.lock(parameter);
    }
    addAndMakeVisible(widget_);
  }

  LockableLabel& lockableLabel() { return lockLabel_; }

  void resized() override {
    auto r = getLocalBounds();
    if (option_ == expand) {
      widget_.setBounds(r);
    } else {
      auto labelBounds = r.removeFromLeft(r.getWidth() / 2);
      lockLabel_.setBounds(labelBounds);
      widget_.setBounds(r);
    }
  }

  void paint(juce::Graphics& ctx) override {
    if (option_ == showLabel) {
      auto r = getLocalBounds();
      r.removeFromRight(r.getWidth() / 2);
      r.removeFromLeft(int(pal_.getFontHeight(TextSize::normal)));

      ctx.setColour(pal_.border().withAlpha(float(0.25)));
      ctx.drawLine({r.getBottomLeft().toFloat(), r.getBottomRight().toFloat()}, pal_.borderWidth());
    }
  }
};

struct LayoutSection {
  juce::String title;
  std::vector<std::shared_ptr<LabeledWidget>> widgets;
};

inline int layoutVerticalSection(std::vector<GroupLabel>& groupLabels, int left, int top,
                                 int sectionWidth, int labelHeight, int labelYIncrement,
                                 const juce::String& groupTitle,
                                 std::vector<std::shared_ptr<juce::Component>>& widgets) {
  using Rect = juce::Rectangle<int>;

  if (groupTitle.isNotEmpty()) {
    groupLabels.emplace_back(groupTitle, Rect{left, top, sectionWidth, labelHeight});
    top += labelYIncrement;
  }

  for (auto& wd : widgets) {
    wd->setBounds(Rect{left, top, sectionWidth, labelHeight});
    top += labelYIncrement;
  }

  return top;
}

inline int layoutActionSection(std::vector<GroupLabel>& groupLabels, int left, int top,
                               int sectionWidth, int labelWidth, int labelXIncrement,
                               int labelHeight, int labelYIncrement, juce::Component& undoButton,
                               juce::Component& redoButton, juce::Component& randomizeButton,
                               juce::Component& presetManager, juce::Component& settingsButton) {
  using Rect = juce::Rectangle<int>;

  groupLabels.emplace_back("Action", Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  undoButton.setBounds(Rect{left, top, labelWidth, labelHeight});

  const int remainingWidth = sectionWidth - labelXIncrement;
  redoButton.setBounds(Rect{left + labelXIncrement, top, remainingWidth, labelHeight});

  top += labelYIncrement;
  randomizeButton.setBounds(Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  groupLabels.emplace_back("Preset & GUI", Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  presetManager.setBounds(Rect{left, top, sectionWidth, labelHeight});

  top += labelYIncrement;
  settingsButton.setBounds(Rect{left, top, sectionWidth, labelHeight});

  return top + labelYIncrement;
}

} // namespace Uhhyou
