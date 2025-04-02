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
  ComboBox<decltype(Scales::amType)> amType;
  ToggleButton<decltype(Scales::boolean)> swapCarriorAndModulator;
  TextKnob<decltype(Scales::unipolar)> carriorSideBandMix;
  TextKnob<decltype(Scales::gain)> outputGain;

private: // Drawing and action items.
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;
};

} // namespace Uhhyou
