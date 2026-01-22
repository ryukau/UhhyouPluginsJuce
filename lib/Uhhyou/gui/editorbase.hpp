// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "widgets.hpp"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Uhhyou {

template<typename ProcessorType> class EditorBase : public juce::AudioProcessorEditor {
public:
#define ACTION_BUTTON (*this), palette, statusBar, numberEditor
  EditorBase(
    ProcessorType &p,
    const juce::String &infoText,
    const juce::String &licenseText,
    std::function<void(void)> randomizeButtonOnClick)
    : AudioProcessorEditor(p)
    , processor(p)
    , numberEditor(palette)
    , statusBar(*this, palette)
    , pluginNameButton(*this, palette, processor.getName(), infoText, licenseText)
    , undoButton(
        ACTION_BUTTON,
        "Undo",
        [&]()
        {
          if (processor.undoManager.canUndo()) processor.undoManager.undo();
        })
    , redoButton(
        ACTION_BUTTON,
        "Redo",
        [&]()
        {
          if (processor.undoManager.canRedo()) processor.undoManager.redo();
        })
    , randomizeButton(ACTION_BUTTON, "Randomize", randomizeButtonOnClick)
    , presetManager((*this), (palette), &(processor.undoManager), processor.param.tree)
  {
    setDefaultColor(lookAndFeel, palette);
    setResizable(true, false);
  }
#undef ACTION_BUTTON

  ~EditorBase() override = default;

  void paint(juce::Graphics &ctx) override
  {
    ctx.setColour(palette.background());
    ctx.fillAll();

    ctx.setColour(palette.foreground());
    for (const auto &x : lines) x.paint(ctx);

    ctx.setFont(palette.getFont(palette.textSizeUi()));
    for (const auto &x : labels) x.paint(ctx);

    auto groupLabelFont = palette.getFont(palette.textSizeUi());
    auto groupLabelMarginWidth
      = juce::GlyphArrangement::getStringWidth(groupLabelFont, "W");
    for (const auto &x : groupLabels) {
      x.paint(ctx, groupLabelFont, 2 * palette.borderThin(), groupLabelMarginWidth);
    }
  }

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    numberEditor.setVisible(false);
  }

protected:
  ProcessorType &processor;
  Palette palette;
  juce::LookAndFeel_V4 lookAndFeel;

  NumberEditor numberEditor;
  StatusBar statusBar;
  PopUpButton pluginNameButton;
  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;
  PresetManager presetManager;

  // `shared_ptr` because inherited class may need to hold references to exchange data.
  std::unordered_map<juce::String, std::shared_ptr<juce::Component>> components;
  std::unordered_map<juce::String, std::vector<LabeledWidget>> sections;
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;

  inline juce::ValueTree getStateTree()
  {
    return processor.param.tree.state.getOrCreateChildWithName("GUI", nullptr);
  }

  void addToSection(
    const juce::String &sectionTitle,
    const juce::String &label,
    juce::Component &component,
    LabeledWidget::Layout option = 0)
  {
    auto [iter, inserted] = sections.try_emplace(sectionTitle);
    iter->second.emplace_back(label, component, option);
  }

  void
  storeComponent(const juce::String &paramId, std::shared_ptr<juce::Component> widget)
  {
    juce::String key = paramId;
    if (!components.try_emplace(key, widget).second) {
      // Create a unique key from the widget's memory address.
      components.emplace(
        key + "#" + juce::String::toHexString((uint64_t)widget.get()), widget);
    }
  }

  template<Style style = Style::common, typename Scale>
  auto addComboBox(
    const juce::String &sectionTitle,
    const juce::String &paramId,
    Scale &scale,
    const std::vector<juce::String> &menuItems,
    const juce::String &label,
    LabeledWidget::Layout layoutOption = LabeledWidget::showLabel)
  {
    auto prm = processor.param.tree.getParameter(paramId);
    auto widget = std::make_shared<ComboBox<Scale, style>>(
      *this, palette, &processor.undoManager, prm, scale, statusBar, numberEditor,
      menuItems);

    storeComponent(paramId, widget);
    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, layoutOption);
    }
    return widget;
  }

  template<Style style = Style::common, typename Scale>
  auto addRotaryTextKnob(
    const juce::String &sectionTitle,
    const std::string &paramId,
    Scale &scale,
    const std::vector<float> snaps,
    int precision = 5,
    const juce::String &label = "",
    LabeledWidget::Layout layoutOption = LabeledWidget::showLabel)
  {
    auto prm = processor.param.tree.getParameter(paramId);
    auto widget = std::make_shared<RotaryTextKnob<Scale, style>>(
      *this, palette, &processor.undoManager, prm, scale, statusBar, numberEditor,
      precision);
    widget->setSnaps(snaps);

    storeComponent(paramId, widget);
    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, layoutOption);
    }
    return widget;
  }

  template<Style style = Style::common, typename Scale>
  auto addTextKnob(
    const juce::String &sectionTitle,
    const std::string &paramId,
    Scale &scale,
    const std::vector<float> snaps,
    int precision = 5,
    const juce::String &label = "",
    LabeledWidget::Layout layoutOption = LabeledWidget::showLabel)
  {
    auto prm = processor.param.tree.getParameter(paramId);
    auto widget = std::make_shared<TextKnob<Scale, style>>(
      *this, palette, &processor.undoManager, prm, scale, statusBar, numberEditor,
      precision);
    widget->setSnaps(snaps);

    storeComponent(paramId, widget);
    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, layoutOption);
    }
    return widget;
  }

  template<typename Scale>
  auto addToggleButton(
    const juce::String &sectionTitle,
    const std::string &paramId,
    Scale &scale,
    const juce::String &label = "",
    LabeledWidget::Layout layoutOption = LabeledWidget::showLabel)
  {
    auto prm = processor.param.tree.getParameter(paramId);
    auto widget = std::make_shared<ToggleButton<Scale>>(
      *this, palette, &processor.undoManager, prm, scale, statusBar, numberEditor, label);

    storeComponent(paramId, widget);
    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, label, *widget, layoutOption);
    }
    return widget;
  }

  std::shared_ptr<XYPad> addXYPad(
    const juce::String &sectionTitle,
    const std::string &paramIdX,
    const std::string &paramIdY,
    const std::vector<float> snapsX,
    const std::vector<float> snapsY)
  {
    auto prmX = processor.param.tree.getParameter(paramIdX);
    auto prmY = processor.param.tree.getParameter(paramIdY);
    auto widget = std::make_shared<XYPad>(
      *this, palette, &processor.undoManager,
      std::array<juce::RangedAudioParameter *, 2>{prmX, prmY}, statusBar);
    widget->setSnaps(snapsX, snapsY);

    storeComponent(paramIdX, widget);
    storeComponent(paramIdY, widget);
    if (sectionTitle.isNotEmpty()) {
      addToSection(sectionTitle, "", *widget, LabeledWidget::expand);
    }
    return widget;
  }
};

} // namespace Uhhyou
