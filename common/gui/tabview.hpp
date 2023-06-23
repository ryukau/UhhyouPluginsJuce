// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "style.hpp"

#include <functional>
#include <vector>

namespace Uhhyou {

class TabView : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabView)

protected:
  struct Tab {
    bool isMouseEntered = false;

    juce::Rectangle<float> rect{};
    juce::String label{};

    std::vector<juce::Component *> components;
    std::function<void(juce::Graphics &ctx)> paintCallback = nullptr;

    Tab(const juce::String &label) : label(label) {}

    void paint(juce::Graphics &ctx)
    {
      if (paintCallback != nullptr) paintCallback(ctx);
    }
  };

  Palette &pal;

  size_t activeTabIndex = 0;
  std::vector<Tab> tabs;
  float tabHeight = float(20);

  bool isMouseEntered = false;
  juce::Font font;

public:
  TabView(Palette &palette, std::vector<juce::String> tabNames) : pal(palette)
  {
    tabs.reserve(tabNames.size());
    for (size_t idx = 0; idx < tabNames.size(); ++idx) {
      tabs.push_back(Tab(tabNames[idx]));
    }
  }

  virtual ~TabView() override {}

  void addWidget(size_t tabIndex, juce::Component *component)
  {
    if (component == nullptr || tabIndex >= tabs.size()) return;
    addChildComponent(component);
    tabs[tabIndex].components.push_back(component);
  }

  void refreshTab()
  {
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      bool isVisible = idx == activeTabIndex;
      for (auto &cmp : tabs[idx].components) cmp->setVisible(isVisible);
    }
  }

  virtual void resized() override
  {
    tabHeight = float(2) * pal.textSizeUi();
    font = pal.getFont(pal.textSizeUi());

    if (tabs.size() <= 0) return;
    const float tabWidth = float(getWidth()) / float(tabs.size());
    for (int idx = 0; idx < tabs.size(); ++idx) {
      tabs[idx].rect.setBounds(float(idx) * tabWidth, float(0), tabWidth, tabHeight);
    }
  }

  juce::Rectangle<int> getInnerBounds()
  {
    int tabHeightI = int(tabHeight);
    return juce::Rectangle<int>{
      tabHeightI, 2 * tabHeightI, getWidth() - 2 * tabHeightI,
      getHeight() - 3 * tabHeightI};
  }

  virtual void paint(juce::Graphics &ctx) override
  {
    const float lw1 = pal.borderThin(); // Border width.
    // const float lw2 = 2 * lw1;
    const float lwHalf = std::ceil(lw1 / 2);
    const float width = float(getWidth());
    const float height = float(getHeight());

    juce::PathStrokeType borderStroke{
      lw1, juce::PathStrokeType::JointStyle::curved,
      juce::PathStrokeType::EndCapStyle::rounded};

    // Inactive tab.
    ctx.setFont(font);
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      if (idx == activeTabIndex) continue;
      const auto &tab = tabs[idx];

      ctx.setColour(pal.boxBackground());
      ctx.fillRect(tab.rect);
      if (tab.isMouseEntered) {
        ctx.setColour(pal.overlayHighlight());
        ctx.fillRect(tab.rect);
      }

      juce::Path path;
      const float tabLeft
        = idx == 0 ? tab.rect.getX() + lwHalf : tab.rect.getX() - lwHalf;
      const float tabRight = idx >= tabs.size() - 1 ? tab.rect.getRight() - lwHalf
                                                    : tab.rect.getRight() + lwHalf;
      const float tabTop = tab.rect.getY() + lwHalf;
      path.startNewSubPath(tabLeft, tabTop);
      path.lineTo(tabRight, tabTop);
      path.lineTo(tabRight, tab.rect.getBottom());
      path.lineTo(tabLeft, tab.rect.getBottom());
      path.closeSubPath();

      ctx.setColour(pal.border());
      ctx.strokePath(path, borderStroke);

      ctx.setColour(pal.foregroundInactive());
      ctx.drawText(tab.label, tab.rect, juce::Justification::centred);
    }

    // Active tab.
    const auto &activeTab = tabs[activeTabIndex];
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
    ctx.drawText(activeTab.label, activeTab.rect, juce::Justification::centred);

    tabs[activeTabIndex].paint(ctx);
  }

  virtual void mouseDrag(const juce::MouseEvent &) override {}
  virtual void mouseUp(const juce::MouseEvent &) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

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

  virtual void mouseMove(const juce::MouseEvent &event) override
  {
    for (auto &tab : tabs) {
      tab.isMouseEntered = tab.rect.contains(event.position);
    }
    repaint();
  }

  virtual void mouseDown(const juce::MouseEvent &event) override
  {
    for (size_t idx = 0; idx < tabs.size(); ++idx) {
      if (tabs[idx].rect.contains(event.position)) {
        activeTabIndex = idx;
        break;
      }
    }
    refreshTab();
    repaint();
  }

  virtual void mouseWheelMove(
    const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
  {
    if (wheel.deltaY == float(0) || event.position.getY() > float(tabHeight)) return;
    if (wheel.deltaY > float(0)) {
      activeTabIndex -= 1;
      if (activeTabIndex >= tabs.size()) activeTabIndex = tabs.size() - 1;
    } else {
      activeTabIndex += 1;
      if (activeTabIndex >= tabs.size()) activeTabIndex = 0;
    }
    refreshTab();
    repaint();
  }
};

} // namespace Uhhyou
