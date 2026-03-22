// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "numbereditor.hpp"
#include "style.hpp"

#include <format>
#include <functional>
#include <optional>
#include <vector>

namespace Uhhyou {

class TabView : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabView)

  class TabViewAccessibilityHandler : public juce::AccessibilityHandler {
  public:
    explicit TabViewAccessibilityHandler(TabView& viewToWrap)
        : juce::AccessibilityHandler(viewToWrap, juce::AccessibilityRole::group),
          view_(viewToWrap) {}

    juce::String getTitle() const override {
      if (view_.tabs_.empty()) { return "Tabs"; }
      return std::format("Tab: {} ({} of {})", view_.tabs_[view_.activeTabIndex_].label.toRawUTF8(),
                         view_.activeTabIndex_ + 1, view_.tabs_.size());
    }

    juce::String getHelp() const override { return "Left and right arrow keys to switch tabs."; }

  private:
    TabView& view_;
  };

protected:
  struct Tab {
    bool isMouseEntered = false;

    juce::Rectangle<float> rect{};
    juce::String label{};

    std::vector<juce::Component*> components;
    std::function<void(juce::Graphics& ctx)> paintCallback = nullptr;

    Tab(const juce::String& lbl) : label(lbl) {}

    void paint(juce::Graphics& ctx) {
      if (paintCallback != nullptr) { paintCallback(ctx); }
    }
  };

  Palette& pal_;
  StatusBar& statusBar_;

  size_t activeTabIndex_ = 0;
  std::vector<Tab> tabs_;
  float tabHeight_ = float(20);

  bool isMouseEntered_ = false;
  juce::Font font_;

  void setActiveTab(size_t newIndex) {
    if (activeTabIndex_ == newIndex) { return; }
    activeTabIndex_ = newIndex;

    refreshTab();
    repaint();

    if (auto* handler = getAccessibilityHandler()) {
      handler->notifyAccessibilityEvent(juce::AccessibilityEvent::titleChanged);
    }
  }

  void selectPreviousTab() {
    size_t newIndex = activeTabIndex_ - 1;
    if (newIndex >= tabs_.size()) { newIndex = tabs_.size() - 1; }
    setActiveTab(newIndex);
  }

  void selectNextTab() {
    size_t newIndex = activeTabIndex_ + 1;
    if (newIndex >= tabs_.size()) { newIndex = 0; }
    setActiveTab(newIndex);
  }

public:
  TabView(Palette& palette, StatusBar& statusBar, std::vector<juce::String> tabNames)
      : pal_(palette), statusBar_(statusBar), font_(juce::FontOptions{}) {
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    tabs_.reserve(tabNames.size());
    for (size_t idx = 0; idx < tabNames.size(); ++idx) { tabs_.push_back(Tab(tabNames[idx])); }
  }

  virtual ~TabView() override {}

  std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override {
    return std::make_unique<TabViewAccessibilityHandler>(*this);
  }

  void addWidget(size_t tabIndex, juce::Component* component) {
    if (component == nullptr || tabIndex >= tabs_.size()) { return; }
    addChildComponent(component);
    tabs_[tabIndex].components.push_back(component);
  }

  void refreshTab() {
    for (size_t idx = 0; idx < tabs_.size(); ++idx) {
      bool isVisible = idx == activeTabIndex_;
      for (auto& cmp : tabs_[idx].components) { cmp->setVisible(isVisible); }
    }
  }

  virtual void resized() override {
    tabHeight_ = float(2) * pal_.getFontHeight(TextSize::normal);
    font_ = pal_.getFont(TextSize::normal);

    if (tabs_.size() <= 0) { return; }
    const float tabWidth = float(getWidth()) / float(tabs_.size());
    for (size_t idx = 0; idx < tabs_.size(); ++idx) {
      tabs_[idx].rect.setBounds(float(idx) * tabWidth, float(0), tabWidth, tabHeight_);
    }
  }

  juce::Rectangle<int> getInnerBounds() {
    int tabHeightI = int(tabHeight_);
    return juce::Rectangle<int>{tabHeightI, 2 * tabHeightI, getWidth() - 2 * tabHeightI,
                                getHeight() - 3 * tabHeightI};
  }

  virtual void paint(juce::Graphics& ctx) override {
    const float lw1 = pal_.borderWidth(); // Border width.
    const float lwHalf = std::ceil(lw1 / 2);
    const float width = float(getWidth());
    const float height = float(getHeight());

    juce::PathStrokeType borderStroke{lw1, juce::PathStrokeType::JointStyle::curved,
                                      juce::PathStrokeType::EndCapStyle::rounded};

    // Inactive tab.
    ctx.setFont(font_);
    for (size_t idx = 0; idx < tabs_.size(); ++idx) {
      if (idx == activeTabIndex_) { continue; }
      const auto& tab = tabs_[idx];

      ctx.setColour(pal_.surface());
      ctx.fillRect(tab.rect);
      if (tab.isMouseEntered) {
        ctx.setColour(pal_.main().withAlpha(0.3f));
        ctx.fillRect(tab.rect);
      }

      juce::Path path;
      const float tabLeft = idx == 0 ? tab.rect.getX() + lwHalf : tab.rect.getX() - lwHalf;
      const float tabRight
        = idx >= tabs_.size() - 1 ? tab.rect.getRight() - lwHalf : tab.rect.getRight() + lwHalf;
      const float tabTop = tab.rect.getY() + lwHalf;
      path.startNewSubPath(tabLeft, tabTop);
      path.lineTo(tabRight, tabTop);
      path.lineTo(tabRight, tab.rect.getBottom());
      path.lineTo(tabLeft, tab.rect.getBottom());
      path.closeSubPath();

      ctx.setColour(pal_.border());
      ctx.strokePath(path, borderStroke);

      ctx.setColour(pal_.foreground().withAlpha(0.5f));
      ctx.drawText(tab.label, tab.rect.toNearestInt(), juce::Justification::centred);
    }

    // Active tab.
    const auto& activeTab = tabs_[activeTabIndex_];
    juce::Path path;
    const float borderedRight = width - lwHalf;
    const float borderedBottom = height - lwHalf;
    const float activeTabLeft = activeTab.rect.getX() + lwHalf;
    const float activeTabRight = activeTab.rect.getRight() - lwHalf;
    path.startNewSubPath(lwHalf, tabHeight_);
    path.lineTo(activeTabLeft, tabHeight_);
    path.lineTo(activeTabLeft, lwHalf);
    path.lineTo(activeTabRight, lwHalf);
    path.lineTo(activeTabRight, tabHeight_);
    path.lineTo(borderedRight, tabHeight_);
    path.lineTo(borderedRight, borderedBottom);
    path.lineTo(lwHalf, borderedBottom);
    path.closeSubPath();

    ctx.setColour(pal_.background());
    ctx.fillPath(path);

    ctx.setColour(pal_.foreground());
    ctx.strokePath(path, borderStroke);
    ctx.drawText(activeTab.label, activeTab.rect.toNearestInt(), juce::Justification::centred);

    tabs_[activeTabIndex_].paint(ctx);

    if (hasKeyboardFocus(false)) {
      auto focusBounds = tabs_[activeTabIndex_].rect.reduced(float(3));
      ctx.drawRect(focusBounds, pal_.borderWidth() * float(2));
    }
  }

  virtual void mouseEnter(const juce::MouseEvent&) override {
    isMouseEntered_ = true;
    repaint();
  }

  virtual void mouseExit(const juce::MouseEvent&) override {
    isMouseEntered_ = false;
    statusBar_.clear();
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
  }

  virtual void mouseMove(const juce::MouseEvent& event) override {
    std::optional<size_t> hovered;
    for (size_t idx = 0; idx < tabs_.size(); ++idx) {
      auto& tab = tabs_[idx];
      tab.isMouseEntered = tab.rect.contains(event.position);
      if (tab.isMouseEntered) { hovered = idx; }
    }
    if (hovered.has_value()) {
      statusBar_.setText("Tab: " + tabs_[*hovered].label);
      if (*hovered != activeTabIndex_) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    } else {
      statusBar_.clear();
      setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent& event) override {
    for (size_t idx = 0; idx < tabs_.size(); ++idx) {
      if (tabs_[idx].rect.contains(event.position)) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        setActiveTab(idx);
        break;
      }
    }
  }

  virtual void mouseWheelMove(const juce::MouseEvent& event,
                              const juce::MouseWheelDetails& wheel) override {
    if (wheel.deltaY == float(0) || event.position.getY() > float(tabHeight_)) { return; }
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
