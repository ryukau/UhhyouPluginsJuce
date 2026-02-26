// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../parameter.hpp"
#include "Uhhyou/dsp/multirate.hpp"
#include "Uhhyou/dsp/smoother.hpp"
#include "transientshaper.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace Uhhyou {

class DSPCore {
public:
  DSPCore(ParameterStore& param) : param(param) {}

  ParameterStore& param;
  bool isPlaying = false;
  double tempo = double(120);
  double beatsElapsed = double(0);
  double timeSigUpper = double(1);
  double timeSigLower = double(4);

  void setup(double sampleRate);
  void reset();
  void startup();
  size_t getLatency();
  void setParameters();
  void process(const size_t length, const float* in0, const float* in1, float* out0, float* out1);

private:
  void updateUpRate();
  std::array<double, 2> processSample(const std::array<double, 2> in);

  static constexpr unsigned upFold = 2;
  unsigned overSampling = 2;
  double sampleRate = 44100;
  double upRate = upFold * 44100.0;

  std::array<float, 2> inputPeakMax{};
  std::array<float, 2> modEnvelopeOutMax{};

  static constexpr double smootherTimeInSecond = double(0.2);
  SmootherParameter<double> smoo;
  ExpSmoother<double> crossoverCutoff{smoo};
  ExpSmoother<double> shaperDecaySample{smoo};
  ExpSmoother<double> shaperRefreshRatio{smoo};
  ExpSmoother<double> shaperIntensity{smoo};
  ExpSmoother<double> shaperPostLowpassCutoff{smoo};
  ExpSmoother<double> lowGain{smoo};
  ExpSmoother<double> highGain{smoo};

  std::array<BandSplitter<double, 63>, 2> splitter;
  std::array<EnvelopeFollowerExpDecay<double>, 2> envelopeHigh;
  std::array<Butterworth<double, 8>, 2> lowpass;

  std::array<std::array<double, 2>, 2> halfbandInput{};
  std::array<HalfBandIIR<double, HalfBandCoefficient<double>>, 2> halfbandIir;
};

} // namespace Uhhyou
