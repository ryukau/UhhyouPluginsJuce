// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <Uhhyou/gui/informationtext.hpp>

#include <juce_core/juce_core.h>

namespace Uhhyou {

const char* informationText = JucePlugin_Name " version " JucePlugin_VersionString R"(

To close this message, click the margin outside of tab view.
)" UHHYOU_TEXT_CONTROLS;

} // namespace Uhhyou
