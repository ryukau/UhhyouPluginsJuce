// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/scale.hpp"
#include "Uhhyou/scaledparameter.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <utility>

namespace Uhhyou {

struct Scales {
  using IntScl = IntScale<float>;
  using UIntScl = UIntScale<float>;
  using LinearScl = LinearScale<float>;
  using DecibelScl = DecibelScale<float>;
  using NegativeDecibelScl = NegativeDecibelScale<float>;
  using BipolarDecibelScl = BipolarDecibelScale<float>;

  UIntScl boolean{1};
  LinearScl unipolar{float(0), float(1)};
  LinearScl bipolar{float(-1), float(1)};

  DecibelScl gain{float(-60), float(60), true};
  DecibelScl crossoverHz{float(20), float(86.0206), false}; // 10-20000 Hz.
};

struct ValueReceivers {
  std::atomic<float> *crossoverHz{};
  std::atomic<float> *upperStereoSpread{};
  std::atomic<float> *lowerStereoSpread{};

  // Internal values used for GUI.
};

class ParameterStore {
public:
  Scales scale;
  ValueReceivers value;

  // `tree` must be defined after `scale` and `value`. Otherwise application will crash
  // due to initialization order. `ValueReceivers` might be excessive abstraction, but
  // it's there to prevent ordering mistake.
  juce::AudioProcessorValueTreeState tree;

private:
  template<typename ParamType>
  inline auto addParameter(
    std::unique_ptr<juce::AudioProcessorParameterGroup> &group, ParamType param)
  {
    auto atomRaw = param->getAtomicRaw();
    group->addChild(std::move(param));
    return atomRaw;
  }

  inline auto createParameterGroup(juce::String name)
  {
    return std::make_unique<juce::AudioProcessorParameterGroup>(name, name, "/");
  }

  auto constructParameter()
  {
    using Cat = juce::AudioProcessorParameter::Category;
    using Rep = ParameterTextRepresentation;

    constexpr int version0 = 0;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto generalGroup = createParameterGroup("generalGroup");

    value.crossoverHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.crossoverHz.invmap(200), scale.crossoverHz, "crossoverHz",
        Cat::genericParameter, version0, "", Rep::raw));
    value.upperStereoSpread = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        1.0f, scale.unipolar, "upperStereoSpread", Cat::genericParameter, version0, "",
        Rep::raw));
    value.lowerStereoSpread = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        1.0f, scale.unipolar, "lowerStereoSpread", Cat::genericParameter, version0, "",
        Rep::raw));

    layout.add(std::move(generalGroup));
    return layout;
  }

public:
  ParameterStore(
    juce::AudioProcessor &processor,
    juce::UndoManager *undoManager,
    const juce::Identifier &id)
    : scale(), value(), tree(processor, undoManager, id, constructParameter())
  {
  }
};

} // namespace Uhhyou
