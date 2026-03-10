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
#include <cstring>
#include <functional>
#include <iterator>
#include <string>
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

  virtual void resized() override { padding = 20 * pal.borderWidth(); }

  virtual void paint(juce::Graphics& ctx) override {
    auto bounds = getLocalBounds().toFloat();

    bool highlight = isMouseEntered || hasKeyboardFocus(false);

    // Background.
    const auto& bgColor = highlight ? pal.main() : pal.background();
    ctx.fillAll(bgColor.withAlpha(float(0.875)));

    // Hint text.
    //
    // Dispalying texts on top and bottom because the available region is narrow.
    // Information density took the priority. A better visual design removes the top text
    // and increases the bottom margin, so that "click to close" text on bottom can be
    // decorated as a clickable button.
    //
    ctx.setColour(highlight ? pal.foreground() : pal.foreground().withAlpha(0.5f));
    ctx.setFont(pal.getFont(TextSize::normal));
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
private:
  Palette& pal;
  juce::String originalSource;
  int currentWrapLimit = 0;

  juce::ScrollBar* getVerticalScrollBar() {
    for (auto* child : getChildren()) {
      if (auto* sb = dynamic_cast<juce::ScrollBar*>(child)) {
        if (sb->isVertical()) { return sb; }
      }
    }
    return nullptr;
  }

  static juce::String prepareManualText(juce::String rawText) {
#if JUCE_MAC
    const juce::String modKey = "Cmd";
#else
    const juce::String modKey = "Ctrl";
#endif

    // Replace token. Do this BEFORE applyLineWrap so the character limits
    // calculate the exact rendered string length accurately.
    return rawText.replace("@MOD@", modKey);
  }

  static juce::String applyLineWrap(const juce::String& text, int limit) {
    if (text.isEmpty() || limit <= 0) { return text; }

    const char* p = text.toRawUTF8();
    const char* const end = p + text.getNumBytesAsUTF8();

    std::string out;
    out.reserve(static_cast<size_t>(end - p) * 2); // Preallocate 200% to avoid resizing

    while (p < end) {
      const char* lineEnd = static_cast<const char*>(std::memchr(p, '\n', end - p));
      if (!lineEnd) { lineEnd = end; }

      int lineChars = 0;
      for (const char* s = p; s < lineEnd; ++s) {
        if ((static_cast<uint8_t>(*s) & 0xC0) != 0x80) { lineChars++; }
      }

      if (lineChars <= limit) {
        out.append(p, lineEnd - p + (lineEnd < end ? 1 : 0));
        p = lineEnd + 1;
        continue;
      }

      const char* scan = p;
      int indentLen = 0;
      while (scan < lineEnd && (*scan == ' ' || *scan == '\t')) {
        indentLen++;
        scan++;
      }

      const int hangingLen = std::max(0, std::min(limit - 10, indentLen + 4));
      out.append(p, scan - p);
      int currentChars = indentLen;

      while (scan < lineEnd) {
        const char* spaceStart = scan;
        while (scan < lineEnd && (*scan == ' ' || *scan == '\t')) { scan++; }

        if (int spaceChars = static_cast<int>(scan - spaceStart);
            spaceChars > 0 && currentChars < limit)
        {
          int writeChars = std::min(spaceChars, limit - currentChars);
          out.append(spaceStart, writeChars);
          currentChars += writeChars;
        }

        if (scan == lineEnd) { break; }

        const char* wordStart = scan;
        int wordChars = 0;
        while (scan < lineEnd && *scan != ' ' && *scan != '\t') {
          if ((static_cast<uint8_t>(*scan) & 0xC0) != 0x80) { wordChars++; }
          scan++;
        }

        if (currentChars > indentLen && currentChars + wordChars > limit) {
          out += '\n';
          out.append(hangingLen, ' ');
          currentChars = hangingLen;
        }

        out.append(wordStart, scan - wordStart);
        currentChars += wordChars;
      }

      if (lineEnd < end) { out += '\n'; }
      p = lineEnd + 1;
    }

    return juce::String::fromUTF8(out.data(), static_cast<int>(out.size()));
  }

public:
  NavigableCodeEditor(juce::CodeDocument& doc, juce::CodeTokeniser* tok, Palette& palette,
                      const juce::String& source)
      : juce::CodeEditorComponent(doc, tok), pal(palette), originalSource(source) {
    setReadOnly(false);

    using ColorId = juce::CodeEditorComponent::ColourIds;
    setColour(ColorId::backgroundColourId, pal.background());
    setColour(ColorId::defaultTextColourId, pal.foreground());
    setColour(ColorId::highlightColourId, pal.main().withAlpha(0.3f));
    setColour(ColorId::lineNumberBackgroundId, pal.background());
    setColour(ColorId::lineNumberTextId, pal.foreground().withAlpha(0.5f));

    setLineNumbersShown(false);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    loadContent(originalSource);
  }

  void updateWrapLimit() {
    auto monospace = pal.getFont(TextSize::normal, FontType::monospace);
    float characterWidth = juce::GlyphArrangement::getStringWidth(monospace, "M");
    if (characterWidth <= 0.0f) { characterWidth = 8.0f; } // Fallback.

    constexpr float scrollBarMargin = 32.0f;
    float availableWidth = static_cast<float>(getWidth()) - scrollBarMargin;
    int limit = static_cast<int>(availableWidth / characterWidth);
    if (limit < 20) { limit = 20; }

    if (limit != currentWrapLimit) {
      currentWrapLimit = limit;

      double scrollRatio = 0.0;
      if (auto* sb = getVerticalScrollBar()) {
        double total = sb->getMaximumRangeLimit() - sb->getMinimumRangeLimit();
        if (total > 0.0) { scrollRatio = sb->getCurrentRangeStart() / total; }
      }
      juce::String wrappedText = applyLineWrap(prepareManualText(originalSource), limit);
      loadContent(wrappedText);

      if (auto* sb = getVerticalScrollBar()) {
        double newTotal = sb->getMaximumRangeLimit() - sb->getMinimumRangeLimit();
        sb->setCurrentRangeStart(scrollRatio * newTotal);
      }
    }
  }

  void resized() override {
    juce::CodeEditorComponent::resized();
    updateWrapLimit();
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
    if (getWantsKeyboardFocus()) { this->grabKeyboardFocus(); }
  }

  void displayPopup() {
    bool wantsFocus = getWantsKeyboardFocus();
    popupScope.setKeyboardFocusEnabled(wantsFocus);

    closeButton.setVisible(true);
    tabView.setVisible(true);

    closeButton.toFront(true);
    tabView.toFront(true);

    navManager.pushScope(this, &popupScope);
    if (wantsFocus) { tabView.grabKeyboardFocus(); }
  }

public:
  PluginInfoButton(Component& parent, Palette& palette, StatusBar& statusBar,
                   NavigationManager& navigationManager, const juce::String& label,
                   const juce::String& infoText, const juce::String& licenseText)
      : popupInset(std::min(int(20 * palette.borderWidth()), 20)),
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
    font = pal.getFont(TextSize::large, FontType::ui);

    popupInset = int(20 * pal.borderWidth());
    closeButton.setBoundsInset(juce::BorderSize<int>{0});
    tabView.setBoundsInset(juce::BorderSize<int>{popupInset});

    auto innerBounds = tabView.getInnerBounds();
    infoDisplay.setBounds(innerBounds);
    infoDisplay.setFont(pal.getFont(TextSize::normal, FontType::monospace));
    licenseDisplay.setBounds(innerBounds);
    licenseDisplay.setFont(pal.getFont(TextSize::normal, FontType::monospace));

    tabView.refreshTab();
  }

  void scale(float scalingFactor) {
    tabView.setBoundsInset(juce::BorderSize<int>{int(scalingFactor * popupInset)});
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal.borderWidth(); // Border width.
    const float lw2 = 2 * lw1;
    const float lwHalf = lw1 / 2;
    const float width = std::floor(float(getWidth()));
    const float height = std::floor(float(getHeight()));

    // Background.
    ctx.setColour(pal.surface());
    ctx.fillRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2);

    // Border.
    ctx.setColour(isMouseEntered ? pal.main() : pal.border());
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
