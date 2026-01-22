// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/scale.hpp"
#include "Uhhyou/scaledparameter.hpp"

#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace Uhhyou {

struct Scales {
  using IntScl = IntScale<float>;
  using UIntScl = UIntScale<float>;
  using LinearScl = LinearScale<float>;
  using DecibelScl = DecibelScale<float>;
  using NegativeDecibelScl = NegativeDecibelScale<float>;
  using BipolarDecibelScl = BipolarDecibelScale<float>;
  using BipolarNegativeDecibelScl = BipolarNegativeDecibelScale<float>;

  UIntScl boolean{1};
  LinearScl unipolar{float(0), float(1)};
  LinearScl bipolar{float(-1), float(1)};
  UIntScl seed{16777215};

  UIntScl saturationType{1023};
  DecibelScl saturationGain{float(-60), float(60), false};
  BipolarDecibelScl gain{float(-60), float(20)};
  DecibelScl delayTimeMs{float(-40), float(60), true};
  BipolarNegativeDecibelScl feedback{float(-60), float(0)};
  DecibelScl lfoBeat{float(-80), float(60), false};
  BipolarDecibelScl timeModOctave{float(-40), float(60)};
  DecibelScl notchWidthHz{float(-60), float(80), false};
  DecibelScl notchTrackingHz{float(-20), float(60), false};
  DecibelScl cutoffHz{float(-20), float(100), true};

  std::vector<std::pair<float, juce::String>> lfoBeatSnaps{
    {1.0f / 6, "1 / 6"}, {1.0f / 5, "1 / 5"}, {1.0f / 4, "1 / 4"}, {1.0f / 3, "1 / 3"},
    {2.0f / 5, "2 / 5"}, {1.0f / 2, "1 / 2"}, {3.0f / 5, "3 / 5"}, {2.0f / 3, "2 / 3"},
    {3.0f / 4, "3 / 4"}, {1.0f, "1"},         {4.0f / 3, "4 / 3"}, {3.0f / 2, "3 / 2"},
    {5.0f / 3, "5 / 3"}, {2.0f, "2"},         {3.0f, "3"},         {4.0f, "4"},
    {5.0f, "5"},         {6.0f, "6"},         {7.0f, "7"},         {8.0f, "8"}};
};

struct ValueReceivers {
  std::atomic<float> *dryGain{};
  std::atomic<float> *wetGain{};
  std::atomic<float> *oversampling{};

  std::atomic<float> *saturationType{};
  std::atomic<float> *saturationGain{};

  std::atomic<float> *delayTimeMs{};
  std::atomic<float> *delayTimeRatio{};
  std::atomic<float> *flangeMode{};
  std::atomic<float> *safeFlange{};
  std::atomic<float> *safeFeedback{};
  std::atomic<float> *feedbackGate{};
  std::atomic<float> *feedback0{};
  std::atomic<float> *feedback1{};
  std::atomic<float> *lfoBeat{};
  std::atomic<float> *stereoPhaseOffset{};
  std::atomic<float> *notchMix{};
  std::atomic<float> *notchWidthHz{};
  std::atomic<float> *notchTrackingHz{};
  std::atomic<float> *highpassCutoffHz{};
  std::atomic<float> *lowpassCutoffHz{};
  std::atomic<float> *rotationToDelayTimeOctave0{};
  std::atomic<float> *rotationToDelayTimeOctave1{};
  std::atomic<float> *crossModulationOctave0{};
  std::atomic<float> *crossModulationOctave1{};

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

    constexpr int version = 0;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto generalGroup = createParameterGroup("generalGroup");

    value.dryGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.gain.invmap(float(0)), scale.gain, "dryGain", Cat::genericParameter,
        version, "", Rep::raw));
    value.wetGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.gain.invmapDB(float(0), +1), scale.gain, "wetGain", Cat::genericParameter,
        version, "", Rep::raw));
    value.oversampling = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(1), scale.boolean, "oversampling", Cat::genericParameter,
        version, "", Rep::raw));

    value.saturationType = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.saturationType.invmap(0), scale.saturationType, "saturationType",
        Cat::genericParameter, version, "", Rep::display));
    value.saturationGain = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.saturationGain.invmapDB(float(0)), scale.saturationGain, "saturationGain",
        Cat::genericParameter, version, "dB", Rep::display));

    value.delayTimeMs = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.delayTimeMs.invmap(1), scale.delayTimeMs, "delayTimeMs",
        Cat::genericParameter, version, "", Rep::raw));
    value.delayTimeRatio = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.unipolar.invmap(float(0.75)), scale.unipolar, "delayTimeRatio",
        Cat::genericParameter, version, "", Rep::raw));
    value.flangeMode = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.unipolar.invmap(float(0)), scale.unipolar, "flangeMode",
        Cat::genericParameter, version, "", Rep::raw));
    value.safeFlange = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(1), scale.boolean, "safeFlange", Cat::genericParameter,
        version, "", Rep::raw));
    value.safeFeedback = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(1), scale.boolean, "safeFeedback", Cat::genericParameter,
        version, "", Rep::raw));
    value.feedbackGate = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::UIntScl>>(
        scale.boolean.invmap(1), scale.boolean, "feedbackGate", Cat::genericParameter,
        version, "", Rep::raw));
    value.feedback0 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarNegativeDecibelScl>>(
        scale.feedback.invmap(float(0)), scale.feedback, "feedback0",
        Cat::genericParameter, version, "", Rep::raw));
    value.feedback1 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarNegativeDecibelScl>>(
        scale.feedback.invmap(float(0)), scale.feedback, "feedback1",
        Cat::genericParameter, version, "", Rep::raw));

    auto lfoBeatToText = [&](float normalized) -> juce::String
    {
      if (normalized >= float(1)) {
        return "Constant";
      }
      const float raw = float(scale.lfoBeat.map(normalized));
      const float eps = std::numeric_limits<float>::epsilon();
      auto isClose = [eps](float a, float b)
      {
        return std::abs(a - b)
          <= eps * std::max(1.0f, std::max(std::abs(a), std::abs(b))) * 10.0f;
      };
      for (const auto &[target, text] : scale.lfoBeatSnaps) {
        if (isClose(raw, target)) return text;
      }
      return formatNumber(raw, std::numeric_limits<float>::digits10 + 1);
    };
    auto lfoBeatFromText = [&](const juce::String &text) -> float
    {
      // "C" is the first character of "constant".
      if (text.startsWithIgnoreCase("C")) return float(1);

      float raw = 0;
      int slashPos = text.indexOfChar('/');
      if (slashPos >= 0) {
        double num = text.substring(0, slashPos).trim().getDoubleValue();
        double den = text.substring(slashPos + 1).trim().getDoubleValue();
        if (std::abs(den) > std::numeric_limits<double>::epsilon()) {
          raw = float(num / den);
        } else {
          if (num != 0) {
            raw = num * den < 0 ? scale.lfoBeat.getMin() : scale.lfoBeat.getMax();
          }
        }
      } else {
        raw = text.getFloatValue();
      }
      return float(scale.lfoBeat.invmap(raw));
    };
    value.lfoBeat = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.lfoBeat.invmap(float(1)), scale.lfoBeat, "lfoBeat", Cat::genericParameter,
        version, "beat", Rep::raw, lfoBeatToText, lfoBeatFromText));

    value.stereoPhaseOffset = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.unipolar.invmap(0), scale.unipolar, "stereoPhaseOffset",
        Cat::genericParameter, version, "", Rep::raw));
    value.notchMix = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.unipolar.invmap(0), scale.unipolar, "notchMix", Cat::genericParameter,
        version, "", Rep::raw));
    value.notchWidthHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.notchWidthHz.invmap(float(1)), scale.notchWidthHz, "notchWidthHz",
        Cat::genericParameter, version, "", Rep::raw));
    value.notchTrackingHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.notchTrackingHz.invmap(1), scale.notchTrackingHz, "notchTrackingHz",
        Cat::genericParameter, version, "", Rep::raw));
    value.lowpassCutoffHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.cutoffHz.invmap(10000.0f), scale.cutoffHz, "lowpassCutoffHz",
        Cat::genericParameter, version, "Hz", Rep::raw));
    value.highpassCutoffHz = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::DecibelScl>>(
        scale.cutoffHz.invmap(10.0f), scale.cutoffHz, "highpassCutoffHz",
        Cat::genericParameter, version, "Hz", Rep::raw));
    value.rotationToDelayTimeOctave0 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.timeModOctave.invmap(0), scale.timeModOctave, "rotationToDelayTimeOctave0",
        Cat::genericParameter, version, "", Rep::raw));
    value.rotationToDelayTimeOctave1 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::LinearScl>>(
        scale.bipolar.invmap(0), scale.bipolar, "rotationToDelayTimeOctave1",
        Cat::genericParameter, version, "", Rep::raw));
    value.crossModulationOctave0 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.timeModOctave.invmap(0), scale.timeModOctave, "crossModulationOctave0",
        Cat::genericParameter, version, "", Rep::raw));
    value.crossModulationOctave1 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.timeModOctave.invmap(0), scale.timeModOctave, "crossModulationOctave1",
        Cat::genericParameter, version, "", Rep::raw));

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
