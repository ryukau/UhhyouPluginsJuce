// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../scaledparameter.hpp"
#include "./numbereditor.hpp"
#include "style.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Uhhyou {

template<typename Scale, Uhhyou::Style style = Uhhyou::Style::common>
class ComboBox : public juce::Component {
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBox)

protected:
  juce::AudioProcessorEditor &editor;
  const juce::RangedAudioParameter *const parameter;

  Scale &scale;
  Palette &pal;
  StatusBar &statusBar;
  NumberEditor &numberEditor;
  juce::ParameterAttachment attachment;

  juce::PopupMenu menu;

  int itemIndex{};
  int defaultIndex{};

  bool isMouseEntered = false;
  juce::Font font;
  std::vector<juce::String> items;

  void updateStatusBar()
  {
    // TODO: Use <format>.
    auto text = parameter->getName(256);
    text += ": ";
    text += items[itemIndex];
    text += " (";
    text += juce::String(itemIndex + 1);
    text += "/";
    text += juce::String(items.size());
    text += ")";
    statusBar.setText(text);
  }

public:
  ComboBox(
    juce::AudioProcessorEditor &editor,
    Palette &palette,
    juce::UndoManager *undoManager,
    juce::RangedAudioParameter *parameter,
    Scale &scale,
    StatusBar &statusBar,
    NumberEditor &numberEditor,
    std::vector<juce::String> menuItems)
    : editor(editor)
    , parameter(parameter)
    , scale(scale)
    , pal(palette)
    , statusBar(statusBar)
    , numberEditor(numberEditor)
    , attachment(
        *parameter,
        [&](float newRaw) {
          int idx = int(std::floor(newRaw) + 0.5f);
          if (itemIndex == idx || idx < 0 || idx > int(items.size())) return;
          itemIndex = idx;
          repaint();
        },
        undoManager)
    , defaultIndex(int(scale.map(parameter->getDefaultValue())))
    , font(palette.getFont(palette.textSizeUi()))
    , items(menuItems)
  {
    attachment.sendInitialUpdate();
    editor.addAndMakeVisible(*this, 0);
  }

  virtual ~ComboBox() override {}

  virtual void resized() override { font = pal.getFont(pal.textSizeUi()); }

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
    if constexpr (style == Uhhyou::Style::accent) {
      ctx.setColour(isMouseEntered ? pal.highlightAccent() : pal.border());
    } else if constexpr (style == Uhhyou::Style::warning) {
      ctx.setColour(isMouseEntered ? pal.highlightWarning() : pal.border());
    } else {
      ctx.setColour(isMouseEntered ? pal.highlightButton() : pal.border());
    }
    ctx.drawRoundedRectangle(lwHalf, lwHalf, width - lw1, height - lw1, lw2, lw1);

    // Text.
    if (itemIndex >= 0 && itemIndex < int(items.size())) {
      ctx.setFont(font);
      ctx.setColour(pal.foreground());
      ctx.drawText(
        items[size_t(itemIndex)],
        juce::Rectangle<float>(float(0), float(0), width, height),
        juce::Justification::centred);
    }
  }

  virtual void mouseMove(const juce::MouseEvent &) override {}

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

  virtual void mouseDown(const juce::MouseEvent &event) override
  {
    if (event.mods.isRightButtonDown()) {
      auto hostContext = editor.getHostContext();
      if (hostContext == nullptr) return;

      auto hostContextMenu = hostContext->getContextMenuForParameter(parameter);
      if (hostContextMenu == nullptr) return;

      hostContextMenu->showNativeMenu(editor.getMouseXYRelative());
      return;
    }

    if (event.mods.isLeftButtonDown()) {
      menu.clear();
      for (size_t idx = 0; idx < items.size(); ++idx) {
        menu.addItem(juce::PopupMenu::Item(items[idx])
                       .setID(int(idx) + 1)
                       .setTicked(int(idx) == itemIndex));
      }

      menu.showMenuAsync(
        juce::PopupMenu::Options().withInitiallySelectedItem(itemIndex + 1), [&](int id) {
          int idx = id - 1;
          if (idx < 0 || idx >= int(items.size())) return;
          itemIndex = idx;
          attachment.setValueAsCompleteGesture(float(itemIndex));
          updateStatusBar();
          repaint();
        });
    }
  }

  virtual void mouseDrag(const juce::MouseEvent &) override {}
  virtual void mouseUp(const juce::MouseEvent &) override {}
  virtual void mouseDoubleClick(const juce::MouseEvent &) override {}

  virtual void
  mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &wheel) override
  {
    if (std::abs(wheel.deltaY) <= std::numeric_limits<float>::epsilon()) return;
    if (items.size() <= 0) return;
    const int size = int(items.size());
    itemIndex = (itemIndex + (wheel.deltaY < 0 ? -1 : 1) + size) % size;
    attachment.setValueAsCompleteGesture(float(itemIndex));
    updateStatusBar();
    repaint();
  }
};

} // namespace Uhhyou
