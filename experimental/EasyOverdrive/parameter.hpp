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
  DecibelScl filterQ{float(-20), float(40), false};
  DecibelScl envelopeSecond{float(-80), float(40), true};

  UIntScl overDriveType{7};
  DecibelScl overDriveHoldSecond{float(-100), float(-20), true};

  DecibelScl asymDriveDecayBias{float(-40), float(40), false};
  LinearScl asymExponentRange{float(0), float(16)};

  UIntScl oversampling{2};
  DecibelScl parameterSmoothingSecond{float(-80), float(40), true};
};

struct ValueReceivers {
  std::atomic<float> *preDriveGain{};
  std::atomic<float> *postDriveGain{};

  std::atomic<float> *overDriveType{};
  std::atomic<float> *overDriveHoldSecond{};
  std::atomic<float> *overDriveQ{};
  std::atomic<float> *overDriveCharacterAmp{};

  std::atomic<float> *asymDriveEnabled{};
  std::atomic<float> *asymDriveDecaySecond{};
  std::atomic<float> *asymDriveDecayBias{};
  std::atomic<float> *asymDriveQ{};
  std::atomic<float> *asymExponentRange{};

  std::atomic<float> *limiterEnabled{};
  std::atomic<float> *limiterInputGain{};
  std::atomic<float> *limiterReleaseSecond{};

  std::atomic<float> *oversampling{};
  std::atomic<float> *parameterSmoothingSecond{};

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
    value.preDriveGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.gain.invmapDB(0.0f), scale.gain, "preDriveGain", Cat::genericParameter,
        version0, "dB"));
    value.postDriveGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.gain.invmapDB(-6.0f), scale.gain, "postDriveGain", Cat::genericParameter,
        version0, "dB"));

    value.overDriveType = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        0.0f, scale.overDriveType, "overDriveType", Cat::genericParameter, version0));
    value.overDriveHoldSecond = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.overDriveHoldSecond.invmap(0.001f), scale.overDriveHoldSecond,
        "overDriveHoldSecond", Cat::genericParameter, version0, "s", Rep::raw));
    value.overDriveQ = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.filterQ.invmap(std::numbers::sqrt2_v<float> / float(2)), scale.filterQ,
        "overDriveQ", Cat::genericParameter, version0, "", Rep::raw));
    value.overDriveCharacterAmp = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.gain.invmapDB(0.0f), scale.gain, "overDriveCharacterAmp",
        Cat::genericParameter, version0, "dB"));

    value.asymDriveEnabled = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        0.0f, scale.boolean, "asymDriveEnabled", Cat::genericParameter, version0));
    value.asymDriveDecaySecond = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.envelopeSecond.invmap(0.01f), scale.envelopeSecond, "asymDriveDecaySecond",
        Cat::genericParameter, version0, "s", Rep::raw));
    value.asymDriveDecayBias = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.asymDriveDecayBias.invmapDB(0.0f), scale.asymDriveDecayBias,
        "asymDriveDecayBias", Cat::genericParameter, version0, "", Rep::raw));
    value.asymDriveQ = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.filterQ.invmap(std::numbers::sqrt2_v<float> / float(2)), scale.filterQ,
        "asymDriveQ", Cat::genericParameter, version0, "", Rep::raw));
    value.asymExponentRange = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.asymExponentRange.invmap(1.0f), scale.asymExponentRange,
        "asymExponentRange", Cat::genericParameter, version0, "", Rep::raw));

    value.limiterEnabled = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        1.0f, scale.boolean, "limiterEnabled", Cat::genericParameter, version0));
    value.limiterInputGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.gain.invmapDB(0.0f), scale.gain, "limiterInputGain", Cat::genericParameter,
        version0, "dB"));
    value.limiterReleaseSecond = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        0.0f, scale.envelopeSecond, "limiterReleaseSecond", Cat::genericParameter,
        version0, "s", Rep::raw));

    value.oversampling = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.oversampling.invmap(1), scale.oversampling, "oversampling",
        Cat::genericParameter, version0));
    value.parameterSmoothingSecond = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.parameterSmoothingSecond.invmap(0.1f), scale.parameterSmoothingSecond,
        "parameterSmoothingSecond", Cat::genericParameter, version0, "s", Rep::raw));
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
