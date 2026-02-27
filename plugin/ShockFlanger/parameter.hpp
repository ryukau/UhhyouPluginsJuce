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

constexpr size_t nChannel = 2;

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
  DecibelScl gain{float(-60), float(60), true};
  DecibelScl delayTimeMs{float(-40), float(80), true};
  BipolarNegativeDecibelScl feedback{float(-60), float(0)};
  DecibelScl lfoBeat{float(-80), float(60), false};
  BipolarDecibelScl timeModOctave{float(-40), float(60)};
  BipolarDecibelScl lfoToDelayTimeOctave{float(-60), float(40)};
  DecibelScl cutoffHz{float(-20), float(100), true};

  LinearScl notePitchRange{float(-1), float(1)};
  LinearScl noteGainRange{float(0), float(60)};

  std::vector<std::pair<float, juce::String>> lfoBeatSnaps{
    {1.0f / 6, "1 / 6"}, {1.0f / 5, "1 / 5"}, {1.0f / 4, "1 / 4"}, {1.0f / 3, "1 / 3"},
    {2.0f / 5, "2 / 5"}, {1.0f / 2, "1 / 2"}, {3.0f / 5, "3 / 5"}, {2.0f / 3, "2 / 3"},
    {3.0f / 4, "3 / 4"}, {1.0f, "1"},         {4.0f / 3, "4 / 3"}, {3.0f / 2, "3 / 2"},
    {5.0f / 3, "5 / 3"}, {2.0f, "2"},         {3.0f, "3"},         {4.0f, "4"},
    {5.0f, "5"},         {6.0f, "6"},         {7.0f, "7"},         {8.0f, "8"},
    {9.0f, "9"},         {10.0f, "10"},       {11.0f, "11"},       {12.0f, "12"},
    {13.0f, "13"},       {14.0f, "14"},       {15.0f, "15"},       {16.0f, "16"},
    {17.0f, "17"},       {18.0f, "18"},       {19.0f, "19"},       {20.0f, "20"},
    {21.0f, "21"},       {22.0f, "22"},       {23.0f, "23"},       {24.0f, "24"},
    {25.0f, "25"},       {26.0f, "26"},       {27.0f, "27"},       {28.0f, "28"},
    {29.0f, "29"},       {30.0f, "30"},       {31.0f, "31"},       {32.0f, "32"},
    {33.0f, "33"},       {34.0f, "34"},       {35.0f, "35"},       {36.0f, "36"},
    {37.0f, "37"},       {38.0f, "38"},       {39.0f, "39"},       {40.0f, "40"},
    {41.0f, "41"},       {42.0f, "42"},       {43.0f, "43"},       {44.0f, "44"},
    {45.0f, "45"},       {46.0f, "46"},       {47.0f, "47"},       {48.0f, "48"},
    {49.0f, "49"},       {50.0f, "50"},       {51.0f, "51"},       {52.0f, "52"},
    {53.0f, "53"},       {54.0f, "54"},       {55.0f, "55"},       {56.0f, "56"},
    {57.0f, "57"},       {58.0f, "58"},       {59.0f, "59"},       {60.0f, "60"},
    {61.0f, "61"},       {62.0f, "62"},       {63.0f, "63"},       {64.0f, "64"}};
};

struct ValueReceivers {
  std::atomic<float>* dryGain{};
  std::atomic<float>* wetGain{};
  std::atomic<float>* wetInvert{};
  std::atomic<float>* oversampling{};

  std::atomic<float>* saturationType{};
  std::atomic<float>* saturationGain{};

  std::atomic<float>* inputRatio{};
  std::atomic<float>* delayTimeMs{};
  std::atomic<float>* delayTimeRatio{};
  std::atomic<float>* flangeMode{};
  std::atomic<float>* safeFlange{};
  std::atomic<float>* safeFeedback{};
  std::atomic<float>* feedbackGate{};
  std::atomic<float>* feedback0{};
  std::atomic<float>* feedback1{};
  std::atomic<float>* lfoBeat{};
  std::atomic<float>* lfoPhaseInitial{};
  std::atomic<float>* lfoPhaseStereoOffset{};
  std::atomic<float>* lfoPhaseReset{};
  std::atomic<float>* highpassCutoffHz{};
  std::atomic<float>* lowpassCutoffHz{};
  std::atomic<float>* modulationTracking{};
  std::atomic<float>* crossModMode{};
  std::atomic<float>* viscosityLowpassHz{};
  std::atomic<float>* crossModulationOctave0{};
  std::atomic<float>* crossModulationOctave1{};
  std::atomic<float>* lfoToDelayTimeOctave0{};
  std::atomic<float>* lfoToDelayTimeOctave1{};
  std::atomic<float>* am0{};
  std::atomic<float>* am1{};

  std::atomic<float>* noteReceive{};
  std::atomic<float>* notePitchRange{};
  std::atomic<float>* noteGainRange{};

  // Internal values used for GUI.
  std::array<std::atomic<float>, nChannel> displayPreSaturationPeak{};
  std::array<std::atomic<float>, nChannel> displayOutputPeak{};
  std::array<std::atomic<float>, nChannel> displayLfoPhase{};
  std::array<std::array<std::atomic<float>, 2>, nChannel> displayDelayTimeUpper{};
  std::array<std::array<std::atomic<float>, 2>, nChannel> displayDelayTimeLower{};
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

    value.dryGain = addParameter(generalGroup,
                                 std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                   scale.gain.invmap(float(0)), scale.gain, "dryGain", "Dry",
                                   Cat::genericParameter, version, "dB", Rep::display));
    value.wetGain = addParameter(generalGroup,
                                 std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                   scale.gain.invmapDB(float(0)), scale.gain, "wetGain", "Wet",
                                   Cat::genericParameter, version, "dB", Rep::display));
    value.wetInvert = addParameter(generalGroup,
                                   std::make_unique<ScaledParameter<Scales::UIntScl>>(
                                     scale.boolean.invmap(0), scale.boolean, "wetInvert",
                                     "Invert Wet", Cat::genericParameter, version, "", Rep::raw));
    value.oversampling
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(1), scale.boolean, "oversampling", "2x Sampling",
                       Cat::genericParameter, version, "", Rep::raw));

    value.saturationType
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.saturationType.invmap(0), scale.saturationType, "saturationType",
                       "Saturation Type", Cat::genericParameter, version, "", Rep::display));
    value.saturationGain = addParameter(generalGroup,
                                        std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                          scale.saturationGain.invmapDB(float(0)),
                                          scale.saturationGain, "saturationGain", "Saturation Gain",
                                          Cat::genericParameter, version, "dB", Rep::display));

    value.inputRatio = addParameter(generalGroup,
                                    std::make_unique<ScaledParameter<Scales::LinearScl>>(
                                      scale.unipolar.invmap(float(0)), scale.unipolar, "inputRatio",
                                      "Input Ratio", Cat::genericParameter, version, "", Rep::raw));
    value.delayTimeMs
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.delayTimeMs.invmap(1), scale.delayTimeMs, "delayTimeMs", "Time",
                       Cat::genericParameter, version, "ms", Rep::raw));
    value.delayTimeRatio
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.unipolar.invmap(float(0.75)), scale.unipolar, "delayTimeRatio",
                       "Time Ratio", Cat::genericParameter, version, "", Rep::raw));
    value.flangeMode = addParameter(generalGroup,
                                    std::make_unique<ScaledParameter<Scales::LinearScl>>(
                                      scale.unipolar.invmap(float(0)), scale.unipolar, "flangeMode",
                                      "Mode", Cat::genericParameter, version, "", Rep::raw));
    value.safeFlange = addParameter(generalGroup,
                                    std::make_unique<ScaledParameter<Scales::UIntScl>>(
                                      scale.boolean.invmap(1), scale.boolean, "safeFlange",
                                      "Safe Flange", Cat::genericParameter, version, "", Rep::raw));
    value.safeFeedback
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(1), scale.boolean, "safeFeedback", "Safe Feedback",
                       Cat::genericParameter, version, "", Rep::raw));
    value.feedbackGate
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(1), scale.boolean, "feedbackGate", "Feedback Gate",
                       Cat::genericParameter, version, "", Rep::raw));
    value.feedback0
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::BipolarNegativeDecibelScl>>(
                       scale.feedback.invmap(float(0)), scale.feedback, "feedback0", "Feedback 0",
                       Cat::genericParameter, version, "", Rep::raw));
    value.feedback1
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::BipolarNegativeDecibelScl>>(
                       scale.feedback.invmap(float(0)), scale.feedback, "feedback1", "Feedback 1",
                       Cat::genericParameter, version, "", Rep::raw));

    auto lfoBeatToText = [&](float normalized) -> juce::String {
      if (normalized >= float(1)) { return "Constant"; }
      const float raw = float(scale.lfoBeat.map(normalized));
      const float eps = std::numeric_limits<float>::epsilon();
      auto isClose = [eps](float a, float b) {
        return std::abs(a - b)
          <= eps * std::max(float(1), std::max(std::abs(a), std::abs(b))) * float(10);
      };
      for (const auto& [target, text] : scale.lfoBeatSnaps) {
        if (isClose(raw, target)) { return text; }
      }
      return formatNumber(raw, std::numeric_limits<float>::digits10 + 1);
    };
    auto lfoBeatFromText = [&](const juce::String& text) -> float {
      // "C" is the first character of "constant".
      if (text.startsWithIgnoreCase("C")) { return float(1); }

      float raw = 0;
      int slashPos = text.indexOfChar('/');
      if (slashPos >= 0) {
        double num = text.substring(0, slashPos).trim().getDoubleValue();
        double den = text.substring(slashPos + 1).trim().getDoubleValue();
        if (std::abs(den) > std::numeric_limits<double>::epsilon()) {
          raw = float(num / den);
        } else {
          if (num != 0) { raw = num * den < 0 ? scale.lfoBeat.getMin() : scale.lfoBeat.getMax(); }
        }
      } else {
        raw = text.getFloatValue();
      }
      return float(scale.lfoBeat.invmap(raw));
    };
    value.lfoBeat = addParameter(generalGroup,
                                 std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                                   scale.lfoBeat.getMax(), scale.lfoBeat, "lfoBeat", "LFO Rate",
                                   Cat::genericParameter, version, "beat", Rep::raw, lfoBeatToText,
                                   lfoBeatFromText));
    value.lfoPhaseInitial
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.unipolar.invmap(0), scale.unipolar, "lfoPhaseInitial", "Initial Phase",
                       Cat::genericParameter, version, "", Rep::raw));
    value.lfoPhaseStereoOffset
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.unipolar.invmap(0), scale.unipolar, "lfoPhaseStereoOffset",
                       "Stereo Phase", Cat::genericParameter, version, "", Rep::raw));
    value.lfoPhaseReset
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(0), scale.boolean, "lfoPhaseReset", "Reset LFO Phase",
                       Cat::genericParameter, version, "", Rep::raw));

    value.lowpassCutoffHz
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.cutoffHz.invmap(float(10000)), scale.cutoffHz, "lowpassCutoffHz",
                       "Lowpass", Cat::genericParameter, version, "Hz", Rep::raw));
    value.highpassCutoffHz
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.cutoffHz.invmap(float(10)), scale.cutoffHz, "highpassCutoffHz",
                       "Highpass", Cat::genericParameter, version, "Hz", Rep::raw));

    value.modulationTracking
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.unipolar.invmap(1), scale.unipolar, "modulationTracking", "Tracking",
                       Cat::genericParameter, version, "", Rep::raw));
    value.crossModMode
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.unipolar.invmap(0), scale.unipolar, "crossModMode", "S. Mod Mode",
                       Cat::genericParameter, version, "", Rep::raw));
    value.viscosityLowpassHz
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::DecibelScl>>(
                       scale.cutoffHz.invmap(float(2000)), scale.cutoffHz, "viscosityLowpassHz",
                       "Viscosity LP", Cat::genericParameter, version, "Hz", Rep::raw));
    value.lfoToDelayTimeOctave0 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.lfoToDelayTimeOctave.invmap(0), scale.lfoToDelayTimeOctave, "lfoToDelayTimeOctave0",
        "LFO Mod. 0", Cat::genericParameter, version, "", Rep::raw));
    value.lfoToDelayTimeOctave1 = addParameter(
      generalGroup,
      std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
        scale.lfoToDelayTimeOctave.invmap(0), scale.lfoToDelayTimeOctave, "lfoToDelayTimeOctave1",
        "LFO Mod. 1", Cat::genericParameter, version, "", Rep::raw));
    value.crossModulationOctave0
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
                       scale.timeModOctave.invmap(0), scale.timeModOctave, "crossModulationOctave0",
                       "S. Mod 0", Cat::genericParameter, version, "", Rep::raw));
    value.crossModulationOctave1
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::BipolarDecibelScl>>(
                       scale.timeModOctave.invmap(0), scale.timeModOctave, "crossModulationOctave1",
                       "S. Mod 1", Cat::genericParameter, version, "", Rep::raw));
    value.am0 = addParameter(generalGroup,
                             std::make_unique<ScaledParameter<Scales::LinearScl>>(
                               scale.unipolar.invmap(0), scale.unipolar, "am0", "AM 0",
                               Cat::genericParameter, version, "", Rep::raw));
    value.am1 = addParameter(generalGroup,
                             std::make_unique<ScaledParameter<Scales::LinearScl>>(
                               scale.unipolar.invmap(0), scale.unipolar, "am1", "AM 1",
                               Cat::genericParameter, version, "", Rep::raw));

    value.noteReceive
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::UIntScl>>(
                       scale.boolean.invmap(0), scale.boolean, "noteReceive", "Recieve Note",
                       Cat::genericParameter, version, "", Rep::display));
    value.notePitchRange = addParameter(generalGroup,
                                        std::make_unique<ScaledParameter<Scales::LinearScl>>(
                                          scale.notePitchRange.invmap(float(1)),
                                          scale.notePitchRange, "notePitchRange", "Note Pitch",
                                          Cat::genericParameter, version, "", Rep::display));
    value.noteGainRange
      = addParameter(generalGroup,
                     std::make_unique<ScaledParameter<Scales::LinearScl>>(
                       scale.noteGainRange.invmap(float(20)), scale.noteGainRange, "noteGainRange",
                       "Note Gain", Cat::genericParameter, version, "", Rep::display));

    layout.add(std::move(generalGroup));
    return layout;
  }

public:
  ParameterStore(juce::AudioProcessor& processor, juce::UndoManager* undoManager,
                 const juce::Identifier& id)
      : scale(), value(), tree(processor, undoManager, id, constructParameter()) {}
};

} // namespace Uhhyou
