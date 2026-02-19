// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "navigation.hpp"
#include "style.hpp"
#include "tabview.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

namespace Uhhyou {

class CloseInfoButton : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloseInfoButton)

  Palette &pal;
  bool isMouseEntered = false;
  float padding{20.0f};

  std::function<void(void)> mouseDownCallback = nullptr;

public:
  CloseInfoButton(
    Palette &palette, int popupInset, std::function<void(void)> mouseDownCallback)
    : pal(palette), mouseDownCallback(mouseDownCallback), padding(float(popupInset))
  {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
  }

  virtual ~CloseInfoButton() override {}

  virtual void paint(juce::Graphics &ctx) override
  {
    auto bounds = getLocalBounds().toFloat();

    bool highlight = isMouseEntered || hasKeyboardFocus(false);

    // Background.
    const auto &bgColor = highlight ? pal.highlightButton() : pal.background();
    ctx.setColour(bgColor.withAlpha(float(0.875)));
    ctx.fillRect(bounds);

    // Hint text.
    //
    // Dispalying texts on top and bottom because the available region is narrow.
    // Information density took the priority. A better visual design removes the top text
    // and increases the bottom margin, so that "click to close" text on bottom can be
    // decorated as a clickable button.
    //
    ctx.setColour(highlight ? pal.foreground() : pal.foreground().withAlpha(0.5f));
    ctx.setFont(pal.getFont(pal.textSizeUi()));
    juce::String hintText = "Click margin to close (Esc)";
    const auto &justification = juce::Justification::centred;
    ctx.drawText(hintText, bounds.withBottom(padding), justification);
    ctx.drawText(hintText, bounds.removeFromBottom(padding), justification);
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

  bool keyPressed(const juce::KeyPress &key) override
  {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::spaceKey) || key.isKeyCode(KP::returnKey)) {
      mouseDownCallback();
      return true;
    }
    return false;
  }

  void focusGained(juce::Component::FocusChangeType) override { repaint(); }
  void focusLost(juce::Component::FocusChangeType) override { repaint(); }
};

class PluginInfoButton : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInfoButton)

  int popupInset{20};

  CloseInfoButton closeButton;
  TabView popup;
  juce::CodeDocument infoDocument{};
  juce::CodeEditorComponent infoDisplay{infoDocument, nullptr};
  juce::CodeDocument licenseDocument{};
  juce::CodeEditorComponent licenseDisplay{licenseDocument, nullptr};

  Palette &pal;
  bool isMouseEntered = false;
  juce::Font font;
  juce::String label;

  NavigationManager &navManager;
  FocusScope popupScope;

  enum TabIndex { information, license };

  juce::Component *getActiveContent()
  {
    if (infoDisplay.isVisible()) return &infoDisplay;
    if (licenseDisplay.isVisible()) return &licenseDisplay;
    return nullptr;
  }

  void dismissPopup()
  {
    navManager.popScope(this);
    closeButton.setVisible(false);
    popup.setVisible(false);
    this->grabKeyboardFocus();
  }

  void displayPopup()
  {
    closeButton.setVisible(true);
    popup.setVisible(true);

    closeButton.toFront(true);
    popup.toFront(true);

    navManager.pushScope(this, &popupScope);
    popup.grabKeyboardFocus();
  }

public:
  PluginInfoButton(
    Component &parent,
    Palette &palette,
    NavigationManager &navigationManager,
    const juce::String &label,
    const juce::String &infoText,
    const juce::String &licenseText)
    : popupInset(std::min(int(20 * palette.borderThin()), 20))
    , closeButton(palette, popupInset, [&]() { this->dismissPopup(); })
    , popup(palette, {"Information", "License"})
    , pal(palette)
    , font(juce::FontOptions{})
    , label(label)
    , navManager(navigationManager)
  {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(false);

    parent.addAndMakeVisible(*this, 0);

    parent.addChildComponent(closeButton, 0);
    closeButton.setBoundsInset(juce::BorderSize<int>{0});

    parent.addChildComponent(popup, -1);
    popup.setBoundsInset(juce::BorderSize<int>{popupInset});

    auto popupAddWidget =
      [&](
        TabIndex tabIndex, juce::CodeEditorComponent &display, const juce::String &source)
    {
      popup.addWidget(tabIndex, &display);

      using ColorId = juce::CodeEditorComponent::ColourIds;
      display.setColour(ColorId::backgroundColourId, pal.background());
      display.setColour(ColorId::defaultTextColourId, pal.foreground());
      display.setColour(ColorId::highlightColourId, pal.overlayHighlight());
      display.setColour(ColorId::lineNumberBackgroundId, pal.background());
      display.setColour(ColorId::lineNumberTextId, pal.foreground().withAlpha(0.5f));

      display.setReadOnly(true);
      display.loadContent(source);
      display.setLineNumbersShown(false);
      display.setWantsKeyboardFocus(true);
    };
    popupAddWidget(TabIndex::information, infoDisplay, infoText);
    popupAddWidget(TabIndex::license, licenseDisplay, licenseText);

    // Setup traversal order.
    popupScope.add(closeButton);
    popupScope.add(popup);
    popupScope.add(infoDisplay);
    popupScope.add(licenseDisplay);
  }

  ~PluginInfoButton() override { navManager.popScope(this); }

  virtual void resized() override
  {
    font = pal.getFont(pal.textSizeBig(), FontType::ui);

    closeButton.setBoundsInset(juce::BorderSize<int>{0});
    popup.setBoundsInset(juce::BorderSize<int>{popupInset});

    auto innerBounds = popup.getInnerBounds();

    infoDisplay.setBounds(innerBounds);
    infoDisplay.setFont(pal.getFont(pal.textSizeUi(), FontType::monospace));

    licenseDisplay.setBounds(innerBounds);
    licenseDisplay.setFont(pal.getFont(pal.textSizeUi(), FontType::monospace));

    popup.refreshTab();
  }

  void scale(float scalingFactor)
  {
    popup.setBoundsInset(juce::BorderSize<int>{int(scalingFactor * popupInset)});
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

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    displayPopup();
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

  bool keyPressed(const juce::KeyPress &key) override
  {
    using KP = juce::KeyPress;
    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      if (hasKeyboardFocus(false)) giveAwayKeyboardFocus();
      displayPopup();
      return true;
    }
    if (key.isKeyCode(KP::escapeKey)) {
      dismissPopup();
      return true;
    }
    return false;
  }
};

} // namespace Uhhyou
