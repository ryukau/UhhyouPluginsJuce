// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

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
  amType = size_t(pv.amType->load());                                                    \
  swapCarriorAndModulator = bool(pv.swapCarriorAndModulator->load());                    \
  carriorSideBandMix.METHOD(pv.carriorSideBandMix->load());                              \
  outputGain.METHOD(pv.outputGain->load());

void DSPCore::reset()
{
  ASSIGN_PARAMETER(reset);

  for (auto &x : amNaive) x.reset();
  for (auto &x : amUpperAA) x.reset();
  for (auto &x : amFullAA) x.reset();
  for (auto &x : amUsbNaive) x.reset();
  for (auto &x : amLsbNaive) x.reset();
  for (auto &x : amUsbAA) x.reset();
  for (auto &x : amLsbAA) x.reset();

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() { ASSIGN_PARAMETER(push); }

#define PROCESS_AM(PROCESSOR)                                                            \
  for (size_t i = 0; i < length; ++i) {                                                  \
    auto sideBand0 = PROCESSOR[0].process(inCar0[i], inMod0[i]);                         \
    auto sideBand1 = PROCESSOR[1].process(inCar1[i], inMod1[i]);                         \
    auto mix = carriorSideBandMix.process();                                             \
    auto gain = outputGain.process();                                                    \
    out0[i] = float(gain * std::lerp(double(inCar0[i]), sideBand0, mix));                \
    out1[i] = float(gain * std::lerp(double(inCar1[i]), sideBand1, mix));                \
  }

void DSPCore::process(
  const size_t length,
  const float *inCar0,
  const float *inCar1,
  const float *inMod0,
  const float *inMod1,
  float *out0,
  float *out1)
{
  SmootherCommon<double>::setBufferSize(double(length));

  if (swapCarriorAndModulator) {
    std::swap(inCar0, inCar1);
    std::swap(inMod0, inMod1);
  }

  if (amType == 0) { // Double Side-band (DSB)
    PROCESS_AM(amNaive);
  } else if (amType == 1) { // Upper Side-band (USB)
    PROCESS_AM(amUsbNaive);
  } else if (amType == 2) { // Lower Side-band (LSB)
    PROCESS_AM(amLsbNaive);
  } else if (amType == 3) { // DSB Upper AA
    PROCESS_AM(amUpperAA);
  } else if (amType == 4) { // DSB Full AA
    PROCESS_AM(amFullAA);
  } else if (amType == 5) { // USB AA
    PROCESS_AM(amUsbAA);
  } else if (amType == 6) { // LSB AA
    PROCESS_AM(amLsbAA);
  } else { // Default to DSB.
    PROCESS_AM(amNaive);
  }
}

} // namespace Uhhyou
