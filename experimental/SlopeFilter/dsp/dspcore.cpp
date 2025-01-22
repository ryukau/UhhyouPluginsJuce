// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

constexpr double limiterAttackSecond = 0.001;

template<typename T> T decibelToAmp(T dB) { return std::pow(T(10), dB / T(20)); }

void DSPCore::setup(double sampleRate_)
{
  sampleRate = double(sampleRate_);

  SmootherCommon<double>::setSampleRate(sampleRate);
  SmootherCommon<double>::setTime(double(0.1));

  reset();
  startup();
}

size_t DSPCore::getLatency() { return 0; }

#define ASSIGN_PARAMETER(METHOD)                                                         \
  auto &pv = param.value;                                                                \
                                                                                         \
  const auto startHz = pv.startHz->load();                                               \
  const auto slopeDecibel = pv.slopeDecibel->load();                                     \
  const auto outputGain = pv.outputGain->load();                                         \
  const bool isHighshelf = bool(pv.shelvingType->load());                                \
  for (auto &x : slopeFilter) {                                                          \
    x.METHOD(sampleRate, startHz, slopeDecibel, outputGain, isHighshelf);                \
  }

void DSPCore::reset()
{
  ASSIGN_PARAMETER(reset);
  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() { ASSIGN_PARAMETER(push); }

void DSPCore::process(
  const size_t length, const float *in0, const float *in1, float *out0, float *out1)
{
  SmootherCommon<double>::setBufferSize(double(length));

  for (size_t i = 0; i < length; ++i) {
    out0[i] = float(slopeFilter[0].process(in0[i]));
    out1[i] = float(slopeFilter[1].process(in1[i]));
  }
}

} // namespace Uhhyou
