// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/widgets.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public juce::AudioProcessorEditor {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor &);
  ~Editor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    numberEditor.setVisible(false);
  }

private: // JUCE related internals.
  Processor &processor;
  Palette palette;
  juce::LookAndFeel_V4 lookAndFeel;

private: // Action items.
  StatusBar statusBar;
  NumberEditor numberEditor;
  PopUpButton pluginNameButton;
  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;
  std::unique_ptr<juce::FileChooser> fileChooser;
  PresetManager presetManager;

private: // Controls tied to parameters.
  TextKnob<decltype(Scales::crossoverHz)> crossoverHz;
  TextKnob<decltype(Scales::unipolar)> upperStereoSpread;
  TextKnob<decltype(Scales::unipolar)> lowerStereoSpread;

private: // Drawing items.
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;
};

} // namespace Uhhyou
