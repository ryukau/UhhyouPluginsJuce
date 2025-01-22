// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

void DSPCore::setup(double sampleRate) {
  sampleRate_ = double(sampleRate);

  smoo_.setTime(sampleRate_, double(1));

  reset();
  startup();
}

size_t DSPCore::getLatency() { return crossoverFilter_[0].getLatency(); }

#define ASSIGN_PARAMETER(METHOD)                                                                   \
  auto& pv = param.value;                                                                          \
                                                                                                   \
  crossoverFreq_.METHOD(pv.crossoverHz->load() / sampleRate_);                                     \
  lowerStereoSpread_.METHOD(pv.lowerStereoSpread->load());                                         \
  upperStereoSpread_.METHOD(pv.upperStereoSpread->load());

void DSPCore::reset() {
  ASSIGN_PARAMETER(reset);

  for (auto& x : crossoverFilter_) { x.reset(); }

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() { ASSIGN_PARAMETER(push); }

template<typename Sample>
inline std::array<Sample, 2> mixStereo(Sample inL, Sample inR, Sample spread) {
  auto mid = (inL + inR) / Sample(2);

  auto outL = std::lerp(mid, inL, spread);
  auto outR = std::lerp(mid, inR, spread);

  return {outL, outR};
}

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  for (size_t i = 0; i < length; ++i) {
    crossoverFreq_.process();
    for (auto& x : crossoverFilter_) { x.prepare(crossoverFreq_.value()); }

    crossoverFilter_[0].process(double(in0[i]));
    crossoverFilter_[1].process(double(in1[i]));

    auto lower = mixStereo(crossoverFilter_[0].output[0], crossoverFilter_[1].output[0],
                           lowerStereoSpread_.process());
    auto upper = mixStereo(crossoverFilter_[0].output[1], crossoverFilter_[1].output[1],
                           upperStereoSpread_.process());

    out0[i] = float(lower[0] + upper[0]);
    out1[i] = float(lower[1] + upper[1]);
  }
}

} // namespace Uhhyou
