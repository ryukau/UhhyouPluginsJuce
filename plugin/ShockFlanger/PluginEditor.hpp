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
  ~Editor() override {};

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  decltype(processor.param.value)& v_() { return processor.param.value; }

  LfoPhaseDisplay lfoPhaseDisplay{*this, palette, v_().displayLfoPhase};
  DelayTimeDisplay delayTimeDisplay{*this, palette, v_()};
  MeterDisplay meterPreSaturationPeak{*this, palette, v_().displayPreSaturationPeak, "Pre-Sat."};
  MeterDisplay meterOutputPeak{*this, palette, v_().displayOutputPeak, "Output"};
};

} // namespace Uhhyou
