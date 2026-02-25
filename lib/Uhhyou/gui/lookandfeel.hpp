// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

#include <algorithm>
#include <cmath>

namespace Uhhyou {

class LookAndFeel : public juce::LookAndFeel_V4 {
private:
  Palette& pal;

  static constexpr float tooltipLineSpacing = 1.125f;

  void layoutTooltipText(juce::TextLayout& layout, const juce::String& text, juce::Font font,
                         float maxWidth, juce::Colour textColour) const {
    juce::AttributedString s;
    s.setJustification(juce::Justification::centredLeft);
    s.append(text.trim(), font, textColour);

    layout.createLayout(s, maxWidth);
    if (layout.getNumLines() > 1) {
      float yPos = 0.0f;
      const float rowHeight = font.getHeight() * tooltipLineSpacing;
      for (auto& line : layout) {
        line.lineOrigin.y = yPos + line.ascent;
        yPos += rowHeight;
      }
      layout.recalculateSize();
    }
  }

public:
  LookAndFeel(Palette& palette) : pal(palette) {
    setColour(juce::CaretComponent::caretColourId, pal.foreground());

    setColour(juce::TextEditor::backgroundColourId, pal.background());
    setColour(juce::TextEditor::textColourId, pal.foreground());
    setColour(juce::TextEditor::highlightColourId, pal.overlayHighlight());
    setColour(juce::TextEditor::highlightedTextColourId, pal.foreground());
    setColour(juce::TextEditor::outlineColourId, pal.border());
    setColour(juce::TextEditor::focusedOutlineColourId, pal.highlightMain());

    setColour(juce::PopupMenu::backgroundColourId, pal.boxBackground());
    setColour(juce::PopupMenu::textColourId, pal.foreground());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, pal.highlightButton());
    setColour(juce::PopupMenu::highlightedTextColourId, pal.foreground());
    setColour(juce::PopupMenu::headerTextColourId, pal.foregroundInactive());

    setColour(juce::TooltipWindow::backgroundColourId, pal.background());
    setColour(juce::TooltipWindow::outlineColourId, pal.foreground());
    setColour(juce::TooltipWindow::textColourId, pal.foreground());

    juce::LookAndFeel::setDefaultLookAndFeel(this);
  }

  juce::Font getPopupMenuFont() override { return pal.getFont(18); }

  juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos,
                                        juce::Rectangle<int> parentArea) override {
    if (tipText.isEmpty()) { return {}; }

    const auto textSize = pal.textSizeUi();
    const auto em = static_cast<int>(std::ceil(textSize));
    juce::Font font = pal.getFont(textSize);

    juce::TextLayout tl;
    layoutTooltipText(tl, tipText, font, 480.0f, findColour(juce::TooltipWindow::textColourId));

    int w = static_cast<int>(std::ceil(tl.getWidth())) + em * 4;
    int h = static_cast<int>(std::ceil(tl.getHeight())) + em * 3;

    int x = (screenPos.x > parentArea.getCentreX()) ? screenPos.x - (w + 12) : screenPos.x + 24;
    int y = (screenPos.y > parentArea.getCentreY()) ? screenPos.y - (h + 6) : screenPos.y + 24;

    return juce::Rectangle<int>(x, y, w, h).constrainedWithin(parentArea);
  }

  void drawTooltip(juce::Graphics& ctx, const juce::String& text, int width, int height) override {
    if (text.isEmpty()) { return; }

    const auto textSize = pal.textSizeUi();
    const auto em = static_cast<int>(std::ceil(textSize));
    juce::Rectangle<int> bounds(width, height);

    auto beveledRect = [&](juce::Rectangle<float> r) -> juce::Path {
      const auto bv = em * 0.5f;
      const auto cv = bv * 1.5f;

      juce::Path p;
      p.startNewSubPath(r.getTopLeft().translated(cv, 0.0f));
      p.lineTo(r.getTopLeft().translated(bv, bv));
      p.lineTo(r.getTopLeft().translated(0.0f, cv));
      p.lineTo(r.getBottomLeft().translated(0.0f, -cv));
      p.lineTo(r.getBottomLeft().translated(bv, -bv));
      p.lineTo(r.getBottomLeft().translated(cv, 0.0f));
      p.lineTo(r.getBottomRight().translated(-cv, 0.0f));
      p.lineTo(r.getBottomRight().translated(-bv, -bv));
      p.lineTo(r.getBottomRight().translated(0.0f, -cv));
      p.lineTo(r.getTopRight().translated(0.0f, cv));
      p.lineTo(r.getTopRight().translated(-bv, bv));
      p.lineTo(r.getTopRight().translated(-cv, 0.0f));
      p.closeSubPath();
      return p;
    };

    ctx.setColour(findColour(juce::TooltipWindow::outlineColourId).withAlpha(0.25f));
    ctx.fillRect(bounds);

    ctx.setColour(findColour(juce::TooltipWindow::backgroundColourId));
    auto bgPath = beveledRect(bounds.reduced(em / 2).toFloat());
    ctx.fillPath(bgPath);

    auto textArea
      = bounds.reduced(2 * em, int(1.5f * em)).translated(int(0.125f * em), 0).toFloat();
    juce::TextLayout tl;
    layoutTooltipText(tl, text, pal.getFont(textSize), textArea.getWidth(),
                      findColour(juce::TooltipWindow::textColourId));

    float yOffset = (textArea.getHeight() - tl.getHeight()) * 0.5f;
    tl.draw(ctx, textArea.translated(0.0f, yOffset));
  }
};

} // namespace Uhhyou
