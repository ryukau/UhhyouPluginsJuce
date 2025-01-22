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

  UIntScl shelvingType{1};
  DecibelScl startHz{float(20), float(86.84845361644413), false};
  LinearScl slopeDecibel{float(-20), float(20)};
  DecibelScl outputGain{float(-60), float(60), true};
};

struct ValueReceivers {
  std::atomic<float> *shelvingType{};
  std::atomic<float> *startHz{};
  std::atomic<float> *slopeDecibel{};
  std::atomic<float> *outputGain{};

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

    value.startHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        0.0f, scale.startHz, "startHz", Cat::genericParameter, version0, "Hz", Rep::raw));
    value.slopeDecibel = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.slopeDecibel.invmap(0.0f), scale.slopeDecibel, "slopeDecibel",
        Cat::genericParameter, version0, "dB", Rep::raw));
    value.outputGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.outputGain.invmapDB(0.0f), scale.outputGain, "outputGain",
        Cat::genericParameter, version0, "dB", Rep::display));

    value.shelvingType = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(1), scale.boolean, "shelvingType", Cat::genericParameter,
        version0));
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
