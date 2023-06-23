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
  TextKnob<decltype(Scales::gain)> preDriveGain;
  TextKnob<decltype(Scales::gain)> postDriveGain;

  ComboBox<decltype(Scales::overDriveType)> overDriveType;
  TextKnob<decltype(Scales::overDriveHoldSecond)> overDriveHoldSecond;
  TextKnob<decltype(Scales::filterQ)> overDriveQ;
  TextKnob<decltype(Scales::gain)> overDriveCharacterAmp;

  ToggleButton<decltype(Scales::boolean)> asymDriveEnabled;
  TextKnob<decltype(Scales::envelopeSecond)> asymDriveDecaySecond;
  TextKnob<decltype(Scales::asymDriveDecayBias)> asymDriveDecayBias;
  TextKnob<decltype(Scales::filterQ)> asymDriveQ;

  ToggleButton<decltype(Scales::boolean)> limiterEnabled;
  TextKnob<decltype(Scales::gain)> limiterInputGain;
  TextKnob<decltype(Scales::envelopeSecond)> limiterReleaseSecond;

  ComboBox<decltype(Scales::oversampling)> oversampling;
  TextKnob<decltype(Scales::parameterSmoothingSecond)> parameterSmoothingSecond;

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

  juce::ParameterAttachment oversamplingAttachment;
  juce::ParameterAttachment limiterEnabledAttachment;
};

} // namespace Uhhyou
