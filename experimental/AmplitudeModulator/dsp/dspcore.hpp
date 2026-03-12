// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../parameter.hpp"
#include "./am.hpp"
#include "Uhhyou/dsp/multirate.hpp"
#include "Uhhyou/dsp/smoother.hpp"

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
  void process(const size_t length, const float* inCar0, const float* inCar1, const float* inMod0,
               const float* inMod1, float* out0, float* out1);

private:
  double sampleRate_ = 44100;

  size_t amType_ = 0;
  bool swapCarriorAndModulator_ = false;
  SmootherParameter<double> smoo_;
  ExpSmoother<double> carriorSideBandMix_{smoo_};
  ExpSmoother<double> outputGain_{smoo_};

  // `AA` is short for anti-aliasing. `Naive` means no anti-alising here.
  std::array<AmplitudeModulator<double>, 2> amNaive_;
  std::array<UpperSideBandAmplitudeModulator<double>, 2> amUsbNaive_;
  std::array<LowerSideBandAmplitudeModulator<double>, 2> amLsbNaive_;
  std::array<AmplitudeModulatorUpperAA<double>, 2> amUpperAA_;
  std::array<AmplitudeModulatorFullAA<double>, 2> amFullAA_;
  std::array<UpperSideBandAmplitudeModulatorAA<double>, 2> amUsbAA_;
  std::array<LowerSideBandAmplitudeModulatorAA<double>, 2> amLsbAA_;
};

} // namespace Uhhyou
