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

  reset();
  startup();
}

size_t DSPCore::getLatency() { return 0; }

#define ASSIGN_PARAMETER(METHOD)                                                                   \
  auto& pv = param.value;                                                                          \
                                                                                                   \
  const auto startHz = pv.startHz->load();                                                         \
  const auto slopeDecibel = pv.slopeDecibel->load();                                               \
  const auto outputGain = pv.outputGain->load();                                                   \
  const bool isHighshelf = bool(pv.shelvingType->load());                                          \
  for (auto& x : slopeFilter_) {                                                                   \
    x.METHOD(sampleRate_, startHz, slopeDecibel, outputGain, isHighshelf);                         \
  }

void DSPCore::reset() {
  ASSIGN_PARAMETER(reset);
  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() { ASSIGN_PARAMETER(push); }

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  for (size_t i = 0; i < length; ++i) {
    out0[i] = float(slopeFilter_[0].process(in0[i]));
    out1[i] = float(slopeFilter_[1].process(in1[i]));
  }
}

} // namespace Uhhyou
