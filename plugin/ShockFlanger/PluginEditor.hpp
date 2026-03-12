// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/editorbase.hpp"
#include "gui/delaytimedisplay.hpp"
#include "gui/lfophasedisplay.hpp"
#include "gui/meterdisplay.hpp"

#include "PluginProcessor.hpp"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public EditorBase<Processor> {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor&);
  ~Editor() override {}

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  decltype(processor_.param.value)& val() { return processor_.param.value; }

  LfoPhaseDisplay lfoPhaseDisplay_{*this, palette_, val().displayLfoPhase};
  DelayTimeDisplay delayTimeDisplay_{*this, palette_, val()};
  MeterDisplay meterPreSaturationPeak_{*this, palette_, val().displayPreSaturationPeak, "Pre-Sat."};
  MeterDisplay meterOutputPeak_{*this, palette_, val().displayOutputPeak, "Output"};
};

} // namespace Uhhyou
