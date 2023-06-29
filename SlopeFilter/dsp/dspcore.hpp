// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../../common/dsp/multirate.hpp"
#include "../../common/dsp/basiclimiter.hpp"
#include "../../common/dsp/smoother.hpp"
#include "../../common/dsp/svf.hpp"
#include "../parameter.hpp"
#include "filter.hpp"

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
  double sampleRate = 44100;
  std::array<SlopeFilter<double, 12>, 2> slopeFilter;
};

} // namespace Uhhyou
