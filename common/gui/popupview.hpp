// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"
#include "tabview.hpp"

#include <functional>
#include <vector>

namespace Uhhyou {

class FullScreenButton : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullScreenButton)

  Palette &pal;
  bool isMouseEntered = false;

  std::function<void(void)> mouseDownCallback = nullptr;

public:
  FullScreenButton(Palette &palette, std::function<void(void)> mouseDownCallback)
    : pal(palette), mouseDownCallback(mouseDownCallback)
  {
  }

  virtual ~FullScreenButton() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    // Background.
    ctx.setColour(isMouseEntered ? pal.highlightMain() : pal.overlayFaint());
    ctx.fillRect(getBounds().toFloat());
  }

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    if (mouseDownCallback != nullptr) mouseDownCallback();
    repaint();
  }

  virtual void mouseEnter(const juce::MouseEvent &) override
  {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent &) override
  {
    isMouseEntered = false;
    repaint();
  }
};

class PopUpButton : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopUpButton)

protected:
  static constexpr int popUpInset = 20;

  Palette &pal;

  FullScreenButton dismissButton;
  TabView popUp;
  juce::TextEditor infoDisplay{"Information"};
  juce::TextEditor licenseDisplay{"License"};

  bool isMouseEntered = false;
  juce::Font font;
  juce::String label;

  enum TabIndex { information, license };

public:
  PopUpButton(
    Component &parent,
    Palette &palette,
    const juce::String &label,
    const juce::String &infoText,
    const juce::String &licenseText)
    : dismissButton(
      palette,
      [&]() {
        dismissButton.setVisible(false);
        popUp.setVisible(false);
      })
    , popUp(palette, {"Information", "License"})
    , pal(palette)
    , label(label)
  {
    parent.addAndMakeVisible(*this, 0);

    parent.addChildComponent(dismissButton, 0);
    dismissButton.setBoundsInset(juce::BorderSize<int>{0});

    parent.addChildComponent(popUp, -1);
    popUp.setBoundsInset(juce::BorderSize<int>{popUpInset});

    popUp.addWidget(TabIndex::information, &infoDisplay);
    {
      using ColorId = juce::TextEditor::ColourIds;
      infoDisplay.setColour(ColorId::backgroundColourId, pal.background());
      infoDisplay.setColour(ColorId::textColourId, pal.foreground());
      infoDisplay.setColour(ColorId::highlightColourId, pal.overlayHighlight());
      infoDisplay.setColour(ColorId::highlightedTextColourId, pal.foreground());
      infoDisplay.setColour(ColorId::outlineColourId, juce::Colours::transparentWhite);
      infoDisplay.setColour(
        ColorId::focusedOutlineColourId, juce::Colours::transparentWhite);
      infoDisplay.setColour(ColorId::shadowColourId, pal.foreground());

      infoDisplay.setMultiLine(true);
      infoDisplay.setScrollbarsShown(true);
      infoDisplay.setFont(pal.getFont(pal.textSizeUi()));
      infoDisplay.setText(infoText);
      infoDisplay.setReadOnly(true);
    }

    popUp.addWidget(TabIndex::license, &licenseDisplay);
    {
      using ColorId = juce::TextEditor::ColourIds;
      licenseDisplay.setColour(ColorId::backgroundColourId, pal.background());
      licenseDisplay.setColour(ColorId::textColourId, pal.foreground());
      licenseDisplay.setColour(ColorId::highlightColourId, pal.overlayHighlight());
      licenseDisplay.setColour(ColorId::highlightedTextColourId, pal.foreground());
      licenseDisplay.setColour(ColorId::outlineColourId, juce::Colours::transparentWhite);
      licenseDisplay.setColour(
        ColorId::focusedOutlineColourId, juce::Colours::transparentWhite);
      licenseDisplay.setColour(ColorId::shadowColourId, pal.foreground());

      licenseDisplay.setMultiLine(true);
      licenseDisplay.setScrollbarsShown(true);
      licenseDisplay.setFont(pal.getFont(pal.textSizeUi()));
      licenseDisplay.setText(licenseText);
      licenseDisplay.setReadOnly(true);
    }
  }

  virtual ~PopUpButton() override {}

  virtual void resized() override
  {
    font = pal.getFont(pal.textSizeBig());

    dismissButton.setBoundsInset(juce::BorderSize<int>{0});
    popUp.setBoundsInset(juce::BorderSize<int>{popUpInset});

    auto innerBounds = popUp.getInnerBounds();

    infoDisplay.setBounds(innerBounds);
    infoDisplay.setFont(pal.getFont(pal.textSizeUi()));

    licenseDisplay.setBounds(innerBounds);
    licenseDisplay.setFont(pal.getFont(pal.textSizeUi()));

    popUp.refreshTab();
  }

  void scale(float scalingFactor)
  {
    popUp.setBoundsInset(juce::BorderSize<int>{int(scalingFactor * popUpInset)});
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = pal.borderThin(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = float(getWidth());
    const float height = float(getHeight());

    // Background.
    ctx.setColour(pal.boxBackground());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    ctx.setColour(isMouseEntered ? pal.highlightButton() : pal.border());
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    ctx.setFont(font);
    ctx.setColour(pal.foreground());
    ctx.drawText(
      label, juce::Rectangle<float>(float(0), float(0), width, height),
      juce::Justification::centred);
  }

  virtual void mouseMove(const juce::MouseEvent &) override {}
  virtual void mouseDrag(const juce::MouseEvent &) override {}
  virtual void mouseUp(const juce::MouseEvent &) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override
  {
  }

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    dismissButton.setVisible(true);
    popUp.setVisible(true);
    repaint();
  }

  virtual void mouseEnter(const juce::MouseEvent &) override
  {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent &) override
  {
    isMouseEntered = false;
    repaint();
  }
};

} // namespace Uhhyou
