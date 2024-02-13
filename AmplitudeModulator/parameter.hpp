// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../common/scale.hpp"
#include "../common/scaledparameter.hpp"

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

  UIntScl amType{31};
  DecibelScl gain{float(-60), float(60), true};
};

struct ValueReceivers {
  std::atomic<float> *amType{};
  std::atomic<float> *carriorSideBandMix{};
  std::atomic<float> *outputGain{};
  std::atomic<float> *swapCarriorAndModulator{};

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

    value.amType = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.amType.invmap(0), scale.amType, "amType", Cat::genericParameter, version0));

    value.carriorSideBandMix = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        0.5f, scale.unipolar, "carriorSideBandMix", Cat::genericParameter, version0, "",
        Rep::raw));
    value.outputGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.gain.invmapDB(0.0f), scale.gain, "outputGain", Cat::genericParameter,
        version0, "dB", Rep::display));

    value.swapCarriorAndModulator = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(0), scale.boolean, "swapCarriorAndModulator",
        Cat::genericParameter, version0));

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
