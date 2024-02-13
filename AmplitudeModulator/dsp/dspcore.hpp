// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../../common/dsp/multirate.hpp"
#include "../../common/dsp/basiclimiter.hpp"
#include "../../common/dsp/smoother.hpp"
#include "../../common/dsp/svf.hpp"
#include "../parameter.hpp"
#include "am.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace Uhhyou {

class DSPCore {
public:
  DSPCore(ParameterStore &param) : param(param) {}

  ParameterStore &param;
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
  void process(
    const size_t length,
    const float *inCar0,
    const float *inCar1,
    const float *inMod0,
    const float *inMod1,
    float *out0,
    float *out1);

private:
  double sampleRate = 44100;

  size_t amType = 0;
  bool swapCarriorAndModulator = false;
  ExpSmoother<double> carriorSideBandMix;
  ExpSmoother<double> outputGain;

  // `AA` is short for anti-aliasing. `Naive` means no anti-alising here.
  std::array<AmplitudeModulator<double>, 2> amNaive;
  std::array<UpperSideBandAmplitudeModulator<double>, 2> amUsbNaive;
  std::array<LowerSideBandAmplitudeModulator<double>, 2> amLsbNaive;
  std::array<AmplitudeModulatorUpperAA<double>, 2> amUpperAA;
  std::array<AmplitudeModulatorFullAA<double>, 2> amFullAA;
  std::array<UpperSideBandAmplitudeModulatorAA<double>, 2> amUsbAA;
  std::array<LowerSideBandAmplitudeModulatorAA<double>, 2> amLsbAA;
};

} // namespace Uhhyou
