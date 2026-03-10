// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

#include <format>
#include <functional>
#include <vector>

namespace Uhhyou {

class TabView : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabView)

  class TabViewAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    explicit TabViewAccessibilityHandler(TabView& viewToWrap)
        : juce::AccessibilityHandler(viewToWrap, juce::AccessibilityRole::group), view(viewToWrap) {
    }

    juce::String getTitle() const override {
      if (view.tabs.empty()) { return "Tabs"; }
      return std::format("Tab: {} ({} of {})", view.tabs[view.activeTabIndex].label.toRawUTF8(),
                         view.activeTabIndex + 1, view.tabs.size());
    }

    juce::String getHelp() const override { return "Left and right arrow keys to switch tabs."; }

  private:
    TabView& view;
  };

protected:
  struct Tab {
    bool isMouseEntered = false;

    juce::Rectangle<float> rect{};
    juce::String label{};

    std::vector<juce::Component*> components;
    std::function<void(juce::Graphics& ctx)> paintCallback = nullptr;

    Tab(const juce::String& label) : label(label) {}

    void paint(juce::Graphics& ctx) {
      if (paintCallback != nullptr) { paintCallback(ctx); }
    }
  };

  Palette& pal;

  size_t activeTabIndex = 0;
  std::vector<Tab> tabs;
  float tabHeight = float(20);

  bool isMouseEntered = false;
  juce::Font font;

  void setActiveTab(size_t newIndex) {
    if (activeTabIndex == newIndex) { return; }
    activeTabIndex = newIndex;

    refreshTab();
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void selectPreviousTab() {
    size_t newIndex = activeTabIndex - 1;
    if (newIndex >= tabs.size()) { newIndex = tabs.size() - 1; }
    setActiveTab(newIndex);
  }

  void selectNextTab() {
    size_t newIndex = activeTabIndex + 1;
    if (newIndex >= tabs.size()) { newIndex = 0; }
    setActiveTab(newIndex);
  }

public:
  TabView(Palette& palette, std::vector<juce::String> tabNames)
      : pal(palette), font(juce::FontOptions{}) {
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    tabs.reserve(tabNames.size());
    for (size_t idx = 0; idx < tabNames.size(); ++idx) { tabs.push_back(Tab(tabNames[idx])); }
  }

  virtual ~TabView() override {}

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    return std::make_unique<TabViewAccessibilityHandler>(*this);
  }

  void addWidget(size_t tabIndex, juce::Component* component) {
    if (component == nullptr || tabIndex >= tabs.size()) { return; }
    addChildComponent(component);
    tabs[tabIndex].components.push_back(component);
  }

  void refreshTab() {
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      bool isVisible = idx == activeTabIndex;
      for (auto& cmp : tabs[idx].components) { cmp->setVisible(isVisible); }
    }
  }

  virtual void resized() override {
    tabHeight = float(2) * pal.getFontHeight(TextSize::normal);
    font = pal.getFont(TextSize::normal);

    if (tabs.size() <= 0) { return; }
    const float tabWidth = float(getWidth()) / float(tabs.size());
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      tabs[idx].rect.setBounds(float(idx) * tabWidth, float(0), tabWidth, tabHeight);
    }
  }

  juce::Rectangle<int> getInnerBounds() {
    int tabHeightI = int(tabHeight);
    return juce::Rectangle<int>{tabHeightI, 2 * tabHeightI, getWidth() - 2 * tabHeightI,
                                getHeight() - 3 * tabHeightI};
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal.borderWidth(); // Border width.
    const float lwHalf = std::ceil(lw1 / 2);
    const float width = float(getWidth());
    const float height = float(getHeight());

    juce::PathStrokeType borderStroke{lw1, juce::PathStrokeType::JointStyle::curved,
                                      juce::PathStrokeType::EndCapStyle::rounded};

    // Inactive tab.
    ctx.setFont(font);
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      if (idx == activeTabIndex) { continue; }
      const auto& tab = tabs[idx];

      ctx.setColour(pal.surface());
      ctx.fillRect(tab.rect);
      if (tab.isMouseEntered) {
        ctx.setColour(pal.main().withAlpha(0.3f));
        ctx.fillRect(tab.rect);
      }

      juce::Path path;
      const float tabLeft = idx == 0 ? tab.rect.getX() + lwHalf : tab.rect.getX() - lwHalf;
      const float tabRight
        = idx >= tabs.size() - 1 ? tab.rect.getRight() - lwHalf : tab.rect.getRight() + lwHalf;
      const float tabTop = tab.rect.getY() + lwHalf;
      path.startNewSubPath(tabLeft, tabTop);
      path.lineTo(tabRight, tabTop);
      path.lineTo(tabRight, tab.rect.getBottom());
      path.lineTo(tabLeft, tab.rect.getBottom());
      path.closeSubPath();

      ctx.setColour(pal.border());
      ctx.strokePath(path, borderStroke);

      ctx.setColour(pal.foreground().withAlpha(0.5f));
      ctx.drawText(tab.label, tab.rect.toNearestInt(), juce::Justification::centred);
    }

    // Active tab.
    const auto& activeTab = tabs[activeTabIndex];
    juce::Path path;
    const float borderedRight = width - lwHalf;
    const float borderedBottom = height - lwHalf;
    const float activeTabLeft = activeTab.rect.getX() + lwHalf;
    const float activeTabRight = activeTab.rect.getRight() - lwHalf;
    path.startNewSubPath(lwHalf, tabHeight);
    path.lineTo(activeTabLeft, tabHeight);
    path.lineTo(activeTabLeft, lwHalf);
    path.lineTo(activeTabRight, lwHalf);
    path.lineTo(activeTabRight, tabHeight);
    path.lineTo(borderedRight, tabHeight);
    path.lineTo(borderedRight, borderedBottom);
    path.lineTo(lwHalf, borderedBottom);
    path.closeSubPath();

    ctx.setColour(pal.background());
    ctx.fillPath(path);

    ctx.setColour(pal.foreground());
    ctx.strokePath(path, borderStroke);
    ctx.drawText(activeTab.label, activeTab.rect.toNearestInt(), juce::Justification::centred);

    tabs[activeTabIndex].paint(ctx);

    if (hasKeyboardFocus(false)) {
      auto focusBounds = tabs[activeTabIndex].rect.reduced(float(3));
      ctx.drawRect(focusBounds, pal.borderWidth() * float(2));
    }
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
  }

  virtual void mouseMove(const juce::MouseEvent& event) override {
    bool cursorOnTab = false;
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      tabs[idx].isMouseEntered = tabs[idx].rect.contains(event.position);
      cursorOnTab = cursorOnTab || (idx != activeTabIndex && tabs[idx].isMouseEntered);
    }
    setMouseCursor(cursorOnTab ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      if (tabs[idx].rect.contains(event.position)) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        setActiveTab(idx);
        break;
      }
    }
  }

  virtual void mouseWheelMove(const juce::MouseEvent& event,
                              const juce::MouseWheelDetails& wheel) override {
    if (wheel.deltaY == float(0) || event.position.getY() > float(tabHeight)) { return; }
    if (wheel.deltaY > float(0)) {
      selectPreviousTab();
    } else {
      selectNextTab();
    }
  }

  bool keyPressed(const juce::KeyPress& key) override {
    using KP = juce::KeyPress;

    if (key.isKeyCode(KP::leftKey)) {
      selectPreviousTab();
      return true;
    }
    if (key.isKeyCode(KP::rightKey)) {
      selectNextTab();
      return true;
    }

    return false;
  }

  void focusGained(juce::Component::FocusChangeType) override { repaint(); }
  void focusLost(juce::Component::FocusChangeType) override { repaint(); }
};

} // namespace Uhhyou
