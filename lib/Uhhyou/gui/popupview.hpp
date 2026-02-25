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

  class CloseInfoButtonAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    CloseInfoButtonAccessibilityHandler(CloseInfoButton& btn, juce::AccessibilityActions actions)
        : juce::AccessibilityHandler(btn, juce::AccessibilityRole::button, std::move(actions)) {}

    juce::String getTitle() const override { return "Close Popup"; }
    juce::String getHelp() const override { return "Closes the information view."; }
  };

  Palette& pal;
  bool isMouseEntered = false;
  float padding{float(20)};

  std::function<void(void)> mouseDownCallback = nullptr;

public:
  CloseInfoButton(Palette& palette, int popupInset, std::function<void(void)> mouseDownCallback)
      : pal(palette), mouseDownCallback(mouseDownCallback), padding(float(popupInset)) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
  }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [this]() {
      if (mouseDownCallback != nullptr) { mouseDownCallback(); }
    });
    return std::make_unique<CloseInfoButtonAccessibilityHandler>(*this, std::move(actions));
  }

  virtual void resized() override { padding = 20 * pal.borderThin(); }

  virtual void paint(juce::Graphics& ctx) override {
    auto bounds = getLocalBounds().toFloat();

    bool highlight = isMouseEntered || hasKeyboardFocus(false);

    // Background.
    const auto& bgColor = highlight ? pal.highlightButton() : pal.background();
    ctx.fillAll(bgColor.withAlpha(float(0.875)));

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
    const auto& justification = juce::Justification::centred;
    ctx.drawText(hintText, bounds.withBottom(padding), justification);
    ctx.drawText(hintText, bounds.removeFromBottom(padding), justification);
  }

  virtual void mouseDown(const juce::MouseEvent&) override {
    if (mouseDownCallback != nullptr) { mouseDownCallback(); }
    repaint();
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    repaint();
  }

  bool keyPressed(const juce::KeyPress& key) override {
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

class NavigableCodeEditor : public juce::CodeEditorComponent {
public:
  NavigableCodeEditor(juce::CodeDocument& doc, juce::CodeTokeniser* tok, Palette& pal,
                      const juce::String& source)
      : juce::CodeEditorComponent(doc, tok) {
    setReadOnly(false);

    using ColorId = juce::CodeEditorComponent::ColourIds;
    setColour(ColorId::backgroundColourId, pal.background());
    setColour(ColorId::defaultTextColourId, pal.foreground());
    setColour(ColorId::highlightColourId, pal.overlayHighlight());
    setColour(ColorId::lineNumberBackgroundId, pal.background());
    setColour(ColorId::lineNumberTextId, pal.foreground().withAlpha(0.5f));

    loadContent(source);
    setLineNumbersShown(false);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);
  }

  void insertTextAtCaret(const juce::String&) override {}

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;

    // Allow Tab/Esc to bubble up to parent.
    if (key == KP::tabKey || key == KP::escapeKey) { return false; }

    // Navigation keys.
    if (key.isKeyCode(KP::leftKey) || key.isKeyCode(KP::rightKey) || key.isKeyCode(KP::upKey)
        || key.isKeyCode(KP::downKey) || key.isKeyCode(KP::pageUpKey)
        || key.isKeyCode(KP::pageDownKey) || key.isKeyCode(KP::homeKey) || key.isKeyCode(KP::endKey)
        || (key.isKeyCode('c') && key.getModifiers().isCommandDown())
        || (key.isKeyCode('a') && key.getModifiers().isCommandDown()))
    {
      return juce::CodeEditorComponent::keyPressed(key);
    }

    // Block everything else that changes text.
    return true;
  }

  bool perform(const InvocationInfo& info) override {
    switch (info.commandID) {
      case juce::StandardApplicationCommandIDs::cut:   // fall through.
      case juce::StandardApplicationCommandIDs::paste: // fall through.
      case juce::StandardApplicationCommandIDs::del:   // fall through.
      case juce::StandardApplicationCommandIDs::undo:  // fall through.
      case juce::StandardApplicationCommandIDs::redo:  // fall through.
        return true;
      default:
        return juce::CodeEditorComponent::perform(info);
    }
  }

  void addPopupMenuItems(juce::PopupMenu& menu, const juce::MouseEvent*) override {
    menu.addItem(juce::StandardApplicationCommandIDs::copy, "Copy",
                 !getHighlightedRegion().isEmpty());
    menu.addItem(juce::StandardApplicationCommandIDs::selectAll, "Select All");
  }
};

class PluginInfoButton : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInfoButton)

  class PluginInfoButtonAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    PluginInfoButtonAccessibilityHandler(PluginInfoButton& btn, juce::AccessibilityActions actions)
        : juce::AccessibilityHandler(btn, juce::AccessibilityRole::button, std::move(actions)),
          button(btn) {}

    juce::String getTitle() const override { return button.label; }
    juce::String getHelp() const override { return ", Show plugin information and license."; }

  private:
    PluginInfoButton& button;
  };

  int popupInset{20};

  CloseInfoButton closeButton;
  TabView tabView;
  juce::CodeDocument infoDocument{};
  NavigableCodeEditor infoDisplay;
  juce::CodeDocument licenseDocument{};
  NavigableCodeEditor licenseDisplay;

  Palette& pal;
  StatusBar& statusBar;
  bool isMouseEntered = false;
  juce::Font font;
  juce::String label;

  NavigationManager& navManager;
  FocusScope popupScope;

  enum TabIndex { information, license };

  juce::Component* getActiveContent() {
    if (infoDisplay.isVisible()) { return &infoDisplay; }
    if (licenseDisplay.isVisible()) { return &licenseDisplay; }
    return nullptr;
  }

  void dismissPopup() {
    navManager.popScope(this);
    closeButton.setVisible(false);
    tabView.setVisible(false);
    this->grabKeyboardFocus();
  }

  void displayPopup() {
    closeButton.setVisible(true);
    tabView.setVisible(true);

    closeButton.toFront(true);
    tabView.toFront(true);

    navManager.pushScope(this, &popupScope);
    tabView.grabKeyboardFocus();
  }

public:
  PluginInfoButton(Component& parent, Palette& palette, StatusBar& statusBar,
                   NavigationManager& navigationManager, const juce::String& label,
                   const juce::String& infoText, const juce::String& licenseText)
      : popupInset(std::min(int(20 * palette.borderThin()), 20)),
        closeButton(palette, popupInset, [&]() { this->dismissPopup(); }),
        tabView(palette, {"Information", "License"}),
        infoDisplay(infoDocument, nullptr, palette, infoText),
        licenseDisplay(licenseDocument, nullptr, palette, licenseText), pal(palette),
        statusBar(statusBar), font(juce::FontOptions{}), label(label),
        navManager(navigationManager) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(false);

    parent.addAndMakeVisible(*this, 0);

    parent.addChildComponent(closeButton, 0);
    closeButton.setBoundsInset(juce::BorderSize<int>{0});

    parent.addChildComponent(tabView, -1);
    tabView.setBoundsInset(juce::BorderSize<int>{popupInset});

    auto popupAddWidget = [&](TabIndex tabIndex, juce::CodeEditorComponent& display) {
      tabView.addWidget(tabIndex, &display);
    };
    popupAddWidget(TabIndex::information, infoDisplay);
    popupAddWidget(TabIndex::license, licenseDisplay);

    // Setup traversal order.
    popupScope.add(closeButton);
    popupScope.add(tabView);
    popupScope.add(infoDisplay);
    popupScope.add(licenseDisplay);
  }

  ~PluginInfoButton() override { navManager.popScope(this); }

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    juce::AccessibilityActions actions;
    actions.addAction(juce::AccessibilityActionType::press, [this]() {
      displayPopup();
      repaint();
    });
    return std::make_unique<PluginInfoButtonAccessibilityHandler>(*this, std::move(actions));
  }

  virtual void resized() override {
    font = pal.getFont(pal.textSizeBig(), FontType::ui);

    popupInset = int(20 * pal.borderThin());
    closeButton.setBoundsInset(juce::BorderSize<int>{0});
    tabView.setBoundsInset(juce::BorderSize<int>{popupInset});

    auto innerBounds = tabView.getInnerBounds();

    infoDisplay.setBounds(innerBounds);
    infoDisplay.setFont(pal.getFont(pal.textSizeUi(), FontType::monospace));

    licenseDisplay.setBounds(innerBounds);
    licenseDisplay.setFont(pal.getFont(pal.textSizeUi(), FontType::monospace));

    tabView.refreshTab();
  }

  void scale(float scalingFactor) {
    tabView.setBoundsInset(juce::BorderSize<int>{int(scalingFactor * popupInset)});
  }

  virtual void paint(juce::Graphics& ctx) override {
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
    ctx.drawText(label, juce::Rectangle<float>(float(0), float(0), width, height),
                 juce::Justification::centred);
  }

  virtual void mouseDown(const juce::MouseEvent&) override {
    displayPopup();
    repaint();
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    statusBar.setText("Show plugin information");
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    statusBar.clear();
    repaint();
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;

    if (key.isKeyCode(KP::returnKey) || key.isKeyCode(KP::spaceKey)) {
      if (hasKeyboardFocus(false)) { giveAwayKeyboardFocus(); }
      displayPopup();
      return true;
    }

    if (key.isKeyCode(KP::escapeKey) && tabView.isVisible()) {
      dismissPopup();
      return true;
    }

    return false;
  }
};

} // namespace Uhhyou
