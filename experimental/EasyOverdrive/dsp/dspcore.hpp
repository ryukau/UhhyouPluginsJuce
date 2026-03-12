// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../parameter.hpp"
#include "./basiclimiter.hpp"
#include "./overdrive.hpp"
#include "Uhhyou/dsp/multirate.hpp"
#include "Uhhyou/dsp/smoother.hpp"

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
  std::array<double, 2> processFrame(const std::array<double, 2>& frame);

  static constexpr size_t upFold = 16;
  static constexpr std::array<size_t, 3> fold{1, 2, upFold};

  double sampleRate_ = 44100;
  double upRate_ = upFold * 44100;

  size_t oversampling_ = 1;
  size_t overDriveType_ = 0;
  bool asymDriveEnabled_ = true;
  bool limiterEnabled_ = true;

  SmootherParameter<double> smoo_;
  ExpSmoother<double> preDriveGain_{smoo_};
  ExpSmoother<double> limiterInputGain_{smoo_};
  ExpSmoother<double> postDriveGain_{smoo_};

  std::array<BadLimiter<double>, 2> overDrive_{{{smoo_}, {smoo_}}};
  std::array<AsymmetricDrive<double>, 2> asymDrive_{{{smoo_}, {smoo_}}};
  std::array<BasicLimiter<double>, 2> limiter_;

  std::array<CubicUpSampler<double, upFold>, 2> upSampler_;
  std::array<DecimationLowpass<double, Sos16FoldFirstStage<double>>, 2> decimationLowpass_;
  std::array<HalfBandIIR<double, HalfBandCoefficient<double>>, 2> halfbandIir_;
};

} // namespace Uhhyou
