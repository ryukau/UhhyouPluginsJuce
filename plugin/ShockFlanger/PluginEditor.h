// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/editorbase.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public EditorBase<Processor> {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor &);
  ~Editor() override {};

  void paint(juce::Graphics &) override;
  void resized() override;
};

} // namespace Uhhyou
