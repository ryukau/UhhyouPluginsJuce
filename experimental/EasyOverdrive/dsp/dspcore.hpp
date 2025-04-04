// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../parameter.hpp"
#include "./overdrive.hpp"
#include "Uhhyou/dsp/basiclimiter.hpp"
#include "Uhhyou/dsp/multirate.hpp"
#include "Uhhyou/dsp/smoother.hpp"
#include "Uhhyou/dsp/svf.hpp"

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
    const size_t length, const float *in0, const float *in1, float *out0, float *out1);

private:
  void updateUpRate();
  std::array<double, 2> processFrame(const std::array<double, 2> &frame);

  static constexpr size_t upFold = 16;
  static constexpr std::array<size_t, 3> fold{1, 2, upFold};

  double sampleRate = 44100;
  double upRate = upFold * 44100;

  size_t oversampling = 1;
  size_t overDriveType = 0;
  bool asymDriveEnabled = true;
  bool limiterEnabled = true;

  ExpSmoother<double> preDriveGain;
  ExpSmoother<double> limiterInputGain;
  ExpSmoother<double> postDriveGain;

  std::array<BadLimiter<double>, 2> overDrive;
  std::array<AsymmetricDrive<double>, 2> asymDrive;
  std::array<BasicLimiter<double>, 2> limiter;

  std::array<CubicUpSampler<double, upFold>, 2> upSampler;
  std::array<DecimationLowpass<double, Sos16FoldFirstStage<double>>, 2> decimationLowpass;
  std::array<HalfBandIIR<double, HalfBandCoefficient<double>>, 2> halfbandIir;
};

} // namespace Uhhyou
