// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/editorbase.hpp"
#include "gui/envelopedisplay.hpp"

#include "PluginProcessor.hpp"

namespace Uhhyou {

class Editor final : public EditorBase<Processor> {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor&);
  ~Editor() override {}

  void resized() override;

private:
  decltype(processor.param.value)& v_() { return processor.param.value; }
  EnvelopeDisplay envelopeDisplay{*this, palette, v_().inputPeakMax, v_().modEnvelopeOutMax};
};

} // namespace Uhhyou
