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
  explicit Editor(Processor &);
  ~Editor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private: // JUCE related internals.
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  Processor &processor;
  Palette palette;

private: // Controls tied to parameters.
  ComboBox<decltype(Scales::shelvingType)> shelvingType;
  TextKnob<decltype(Scales::startHz)> startHz;
  TextKnob<decltype(Scales::slopeDecibel)> slopeDecibel;
  TextKnob<decltype(Scales::outputGain)> outputGain;

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
