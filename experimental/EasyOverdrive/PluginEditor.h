// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/uhhyoueditor.hpp"
#include "Uhhyou/gui/widgets.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public UhhyouEditor {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor &);
  ~Editor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

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
  TextKnob<decltype(Scales::asymExponentRange)> asymExponentRange;

  ToggleButton<decltype(Scales::boolean)> limiterEnabled;
  TextKnob<decltype(Scales::gain)> limiterInputGain;
  TextKnob<decltype(Scales::envelopeSecond)> limiterReleaseSecond;

  ComboBox<decltype(Scales::oversampling)> oversampling;
  TextKnob<decltype(Scales::parameterSmoothingSecond)> parameterSmoothingSecond;

private: // Drawing and action items.
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;

  juce::ParameterAttachment oversamplingAttachment;
  juce::ParameterAttachment limiterEnabledAttachment;
};

} // namespace Uhhyou
