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
  Palette& pal_;

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
  LookAndFeel(Palette& palette) : pal_(palette) {
    setColour(juce::CaretComponent::caretColourId, pal_.foreground());

    setColour(juce::ScrollBar::backgroundColourId, pal_.surface());
    setColour(juce::ScrollBar::thumbColourId, pal_.main());
    setColour(juce::ScrollBar::trackColourId, pal_.border());

    setColour(juce::TextEditor::backgroundColourId, pal_.background());
    setColour(juce::TextEditor::textColourId, pal_.foreground());
    setColour(juce::TextEditor::highlightColourId, pal_.main().withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, pal_.foreground());
    setColour(juce::TextEditor::outlineColourId, pal_.border());
    setColour(juce::TextEditor::focusedOutlineColourId, pal_.main());

    setColour(juce::PopupMenu::backgroundColourId, pal_.surface());
    setColour(juce::PopupMenu::textColourId, pal_.foreground());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, pal_.main());
    setColour(juce::PopupMenu::highlightedTextColourId, pal_.foreground());
    setColour(juce::PopupMenu::headerTextColourId, pal_.foreground());

    setColour(juce::TooltipWindow::backgroundColourId, pal_.background());
    setColour(juce::TooltipWindow::outlineColourId, pal_.foreground());
    setColour(juce::TooltipWindow::textColourId, pal_.foreground());

    juce::LookAndFeel::setDefaultLookAndFeel(this);
  }

  juce::Font getPopupMenuFont() override { return pal_.getFont(TextSize::normal); }

  void getIdealPopupMenuSectionHeaderSizeWithOptions(const juce::String& text,
                                                     int standardMenuItemHeight, int& idealWidth,
                                                     int& idealHeight,
                                                     const juce::PopupMenu::Options&) override {
    const int borderPx = std::max(1, juce::roundToInt(pal_.borderWidth()));
    auto font = pal_.getFont(TextSize::large);
    constexpr float scalar = 2.0f;

    if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / scalar) {
      font.setHeight(std::floor(standardMenuItemHeight / scalar));
    }

    idealHeight = standardMenuItemHeight > 0
      ? standardMenuItemHeight
      : juce::roundToInt(font.getHeight() * scalar) + borderPx * 2;
    idealWidth
      = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text)) + idealHeight * 2;
  }

  void drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                  const juce::String& sectionName) override {
    const int em = juce::roundToInt(pal_.getFontHeight(TextSize::normal));
    const int margin = em;

    auto font = pal_.getFont(TextSize::large).boldened();
    g.setFont(font);

    juce::Colour textCol = findColour(juce::PopupMenu::headerTextColourId);
    g.setColour(textCol);

    int fontH = juce::roundToInt(font.getHeight());
    int yOffset = (area.getHeight() - fontH) / 2;
    int startX = area.getX() + margin;

    juce::Rectangle<int> crispRect(startX, area.getY() + yOffset, area.getWidth() - margin * 2,
                                   fontH);
    g.drawFittedText(sectionName, crispRect, juce::Justification::centredLeft, 1);

    int textWidth = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, sectionName));
    int lineStartX = startX + textWidth + margin;
    int lineEndX = area.getRight() - margin;

    if (lineStartX < lineEndX) {
      float centerY = area.getY() + std::floor(area.getHeight() / 2.0f);
      g.setColour(textCol.withAlpha(0.2f));
      g.drawHorizontalLine(juce::roundToInt(centerY), float(lineStartX), float(lineEndX));
    }
  }

  void getIdealPopupMenuItemSize(const juce::String& text, const bool isSeparator,
                                 int standardMenuItemHeight, int& idealWidth,
                                 int& idealHeight) override {
    const int em = juce::roundToInt(pal_.getFontHeight(TextSize::normal));
    const int borderPx = std::max(1, juce::roundToInt(pal_.borderWidth()));

    if (isSeparator) {
      idealWidth = em * 4;
      idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : std::max(2, em / 2);
    } else {
      constexpr float scalar = 1.5f;
      auto font = getPopupMenuFont();

      if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / scalar) {
        font.setHeight(standardMenuItemHeight / scalar);
      }

      idealHeight = standardMenuItemHeight > 0
        ? standardMenuItemHeight
        : juce::roundToInt(font.getHeight() * scalar) + borderPx * 2;
      idealWidth
        = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text)) + idealHeight * 2;
    }
  }

  void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                         const bool isSeparator, const bool isActive, const bool isHighlighted,
                         const bool isTicked, const bool hasSubMenu, const juce::String& text,
                         const juce::String& shortcutKeyText, const juce::Drawable* icon,
                         const juce::Colour* const textColourToUse) override {
    const int em = juce::roundToInt(pal_.getFontHeight(TextSize::normal));
    const int margin = em;
    const int borderPx = std::max(1, juce::roundToInt(pal_.borderWidth()));

    if (isSeparator) {
      g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.2f));
      float centerY = area.getY() + (area.getHeight() / 2.0f);

      int lineStartX = area.getX() + margin;
      int lineEndX = area.getRight() - margin;

      g.drawHorizontalLine(juce::roundToInt(centerY), float(lineStartX), float(lineEndX));
      return;
    }

    auto textColour
      = (textColourToUse == nullptr ? findColour(juce::PopupMenu::textColourId) : *textColourToUse);

    auto r = area.reduced(borderPx);

    if (isHighlighted && isActive) {
      g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
      g.fillRect(r);
      g.setColour(findColour(juce::PopupMenu::highlightedTextColourId));
    } else {
      g.setColour(textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));
    }

    int hPad = juce::jmin(juce::roundToInt(em * 0.35f), area.getWidth() / 20);
    r.reduce(hPad, 0);

    auto font = getPopupMenuFont();
    constexpr float scalar = 1.5f;

    float maxFontHeight = (float)r.getHeight() / scalar;
    if (font.getHeight() > maxFontHeight) { font.setHeight(std::floor(maxFontHeight)); }

    g.setFont(font);

    auto iconArea = r.removeFromLeft(juce::roundToInt(font.getHeight())).toFloat();

    if (icon != nullptr) {
      icon->drawWithin(
        g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
        1.0f);
      r.removeFromLeft(juce::roundToInt(font.getHeight() * 0.5f));
    } else if (isTicked) {
      auto tick = getTickShape(1.0f);
      g.fillPath(tick,
                 tick.getTransformToScaleToFit(
                   iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
    }

    if (hasSubMenu) {
      auto arrowH = 0.6f * font.getAscent();
      auto x = static_cast<float>(r.removeFromRight((int)arrowH).getX());
      auto halfH = static_cast<float>(r.getCentreY());

      juce::Path path;
      path.startNewSubPath(x, halfH - arrowH * 0.5f);
      path.lineTo(x + arrowH * 0.6f, halfH);
      path.lineTo(x, halfH + arrowH * 0.5f);

      // Scaled drawing thickness for the arrow
      float strokeThickness = std::max(1.0f, em * 0.1f);
      g.strokePath(path, juce::PathStrokeType(strokeThickness));
    }

    r.removeFromRight(juce::roundToInt(em * 0.2f));

    int fontH = juce::roundToInt(font.getHeight());
    int yOffset = (r.getHeight() - fontH) / 2;
    juce::Rectangle<int> crispTextRect(r.getX(), r.getY() + yOffset, r.getWidth(), fontH);

    g.drawFittedText(text, crispTextRect, juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty()) {
      auto f2 = font;
      f2.setHeight(std::floor(f2.getHeight() * 0.75f));
      f2.setHorizontalScale(0.95f);
      g.setFont(f2);

      int f2H = juce::roundToInt(f2.getHeight());
      int f2yOffset = (r.getHeight() - f2H) / 2;
      juce::Rectangle<int> crispShortcutRect(r.getX(), r.getY() + f2yOffset, r.getWidth(), f2H);

      g.drawText(shortcutKeyText, crispShortcutRect, juce::Justification::centredRight, true);
    }
  }

  juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos,
                                        juce::Rectangle<int> parentArea) override {
    if (tipText.isEmpty()) { return {}; }

    const float textSize = pal_.getFontHeight(TextSize::normal);
    const auto em = static_cast<int>(std::ceil(textSize));
    juce::Font font = pal_.getFont(TextSize::normal);

    juce::TextLayout tl;
    layoutTooltipText(tl, tipText, font, 480.0f, findColour(juce::TooltipWindow::textColourId));

    int w = static_cast<int>(std::ceil(tl.getWidth())) + em * 4;
    int h = static_cast<int>(std::ceil(tl.getHeight())) + em * 3;

    int x = (screenPos.x > parentArea.getCentreX()) ? screenPos.x - (w + em) : screenPos.x + em * 2;
    int y
      = (screenPos.y > parentArea.getCentreY()) ? screenPos.y - (h + em / 2) : screenPos.y + em * 2;

    return juce::Rectangle<int>(x, y, w, h).constrainedWithin(parentArea);
  }

  void drawTooltip(juce::Graphics& ctx, const juce::String& text, int width, int height) override {
    if (text.isEmpty()) { return; }

    const float textSize = pal_.getFontHeight(TextSize::normal);
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
    layoutTooltipText(tl, text, pal_.getFont(TextSize::normal), textArea.getWidth(),
                      findColour(juce::TooltipWindow::textColourId));

    float yOffset = (textArea.getHeight() - tl.getHeight()) * 0.5f;
    tl.draw(ctx, textArea.translated(0.0f, yOffset));
  }
};

} // namespace Uhhyou
