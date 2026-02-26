// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/scale.hpp"
#include "Uhhyou/scaledparameter.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <numbers>
#include <string>
#include <utility>

namespace Uhhyou {

constexpr size_t nChannel = 2;

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
  UIntScl seed{16777215};

  DecibelScl gain{float(-60), float(60), true};
  DecibelScl cutoff{float(0), float(100), false};
  DecibelScl decay{float(-80), float(40), false};
  DecibelScl refreshRatio{float(-40), float(40), false};
};

struct ValueReceivers {
  std::atomic<float>* lowGain{};
  std::atomic<float>* highGain{};
  std::atomic<float>* oversampling{};
  std::atomic<float>* crossoverHz{};
  std::atomic<float>* shaperDecaySecond{};
  std::atomic<float>* shaperRefreshRatio{};
  std::atomic<float>* shaperIntensity{};
  std::atomic<float>* shaperPostLowpassHz{};

  // Internal values used for GUI.
  std::array<std::atomic<float>, nChannel> inputPeakMax{float(0), float(0)};
  std::array<std::atomic<float>, nChannel> modEnvelopeOutMax{float(0), float(0)};
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
  inline auto addParameter(std::unique_ptr<juce::AudioProcessorParameterGroup>& group,
                           ParamType param) {
    auto atomRaw = param->getAtomicRaw();
    group->addChild(std::move(param));
    return atomRaw;
  }

  inline auto createParameterGroup(juce::String name) {
    return std::make_unique<juce::AudioProcessorParameterGroup>(name, name, "/");
  }

  auto constructParameter() {
    using Cat = juce::AudioProcessorParameter::Category;
    using Rep = ParameterTextRepresentation;

    constexpr int version = 0;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto generalGroup = createParameterGroup("generalGroup");

    value.lowGain = addParameter(generalGroup,
                                 std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                   scale.gain.invmapDB(float(0)), scale.gain, "lowGain", "Low Gain",
                                   Cat::genericParameter, version, "dB", Rep::raw));
    value.highGain = addParameter(generalGroup,
                                  std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                    scale.gain.invmapDB(float(0)), scale.gain, "highGain",
                                    "High Gain", Cat::genericParameter, version, "dB", Rep::raw));
    value.crossoverHz
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.cutoff.invmap(float(400)), scale.cutoff, "crossoverHz", "Crossover",
                       Cat::genericParameter, version, "Hz", Rep::raw));
    value.shaperDecaySecond
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.decay.invmap(float(0.1)), scale.decay, "shaperDecaySecond", "Decay",
                       Cat::genericParameter, version, "s", Rep::raw));
    value.shaperRefreshRatio = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.refreshRatio.invmap(float(2.0)), scale.refreshRatio, "shaperRefreshRatio",
        "Refresh Ratio", Cat::genericParameter, version, "", Rep::raw));
    value.shaperIntensity
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.gain.invmap(float(10.0)), scale.gain, "shaperIntensity", "Intensity",
                       Cat::genericParameter, version, "", Rep::raw));
    value.shaperPostLowpassHz
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.cutoff.invmap(float(2000.0)), scale.cutoff, "shaperPostLowpassHz",
                       "Post-lowpass", Cat::genericParameter, version, "Hz", Rep::raw));
    value.oversampling
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(0), scale.boolean, "oversampling", "2x Sampling",
                       Cat::genericParameter, version, "", Rep::raw));

    layout.add(std::move(generalGroup));
    return layout;
  }

public:
  ParameterStore(juce::AudioProcessor& processor, juce::UndoManager* undoManager,
                 const juce::Identifier& id)
      : scale(), value(), tree(processor, undoManager, id, constructParameter()) {}
};

} // namespace Uhhyou
