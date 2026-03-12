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
  DSPCore(ParameterStore& p) : param(p) {}

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
  unsigned overSampling_ = 2;
  double sampleRate_ = 44100;
  double upRate_ = upFold * 44100.0;

  std::array<float, 2> inputPeakMax_{};
  std::array<float, 2> modEnvelopeOutMax_{};

  static constexpr double smootherTimeInSecond = double(0.2);
  SmootherParameter<double> smoo_;
  ExpSmoother<double> crossoverCutoff_{smoo_};
  ExpSmoother<double> shaperDecaySample_{smoo_};
  ExpSmoother<double> shaperRefreshRatio_{smoo_};
  ExpSmoother<double> shaperIntensity_{smoo_};
  ExpSmoother<double> shaperPostLowpassCutoff_{smoo_};
  ExpSmoother<double> lowGain_{smoo_};
  ExpSmoother<double> highGain_{smoo_};

  std::array<BandSplitter<double, 63>, 2> splitter_;
  std::array<EnvelopeFollowerExpDecay<double>, 2> envelopeHigh_;
  std::array<Butterworth<double, 8>, 2> lowpass_;

  std::array<std::array<double, 2>, 2> halfbandInput_{};
  std::array<HalfBandIIR<double, HalfBandCoefficient<double>>, 2> halfbandIir_;
};

} // namespace Uhhyou
