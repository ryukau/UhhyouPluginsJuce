// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../common/gui/widgets.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public juce::AudioProcessorEditor {
public:
  explicit Editor(Processor &);
  ~Editor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private: // JUCE related internals.
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  Processor &processor;
  Palette palette;

private: // Controls tied to parameters.
  ComboBox<decltype(Scales::amType)> amType;
  ToggleButton<decltype(Scales::boolean)> swapCarriorAndModulator;
  TextKnob<decltype(Scales::unipolar)> carriorSideBandMix;
  TextKnob<decltype(Scales::gain)> outputGain;

private: // Drawing and action items.
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;

  PopUpButton pluginNameButton;

  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;

  std::unique_ptr<juce::FileChooser> fileChooser;

  PresetManager presetManager;
};

} // namespace Uhhyou
