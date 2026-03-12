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
  upRate_ = sampleRate_ * upFold;

  smoo_.setTime(upRate_, smootherTimeInSecond);

  reset();
  startup();
}

void DSPCore::updateUpRate() {
  upRate_ = sampleRate_ * (overSampling_ ? 2 : 1);
  smoo_.setTime(upRate_, smootherTimeInSecond);
}

size_t DSPCore::getLatency() { return splitter_[0].latency; }

#define ASSIGN_PARAMETER(METHOD)                                                                   \
  auto& pv = param.value;                                                                          \
                                                                                                   \
  crossoverCutoff_.METHOD(pv.crossoverHz->load() / upRate_);                                       \
  shaperDecaySample_.METHOD(pv.shaperDecaySecond->load() * upRate_);                               \
  shaperRefreshRatio_.METHOD(pv.shaperRefreshRatio->load());                                       \
  const auto intensity = pv.shaperIntensity->load();                                               \
  shaperIntensity_.METHOD(intensity);                                                              \
  shaperPostLowpassCutoff_.METHOD(pv.shaperPostLowpassHz->load() / upRate_);                       \
  lowGain_.METHOD(pv.lowGain->load());                                                             \
  highGain_.METHOD(pv.highGain->load() / std::sqrt(double(1) + intensity));

void DSPCore::reset() {
  updateUpRate();

  ASSIGN_PARAMETER(reset);

  for (auto& x : pv.inputPeakMax) { x = float(0); }
  for (auto& x : pv.modEnvelopeOutMax) { x = float(0); }

  for (auto& x : splitter_) { x.reset(); }
  for (auto& x : envelopeHigh_) { x.reset(); }
  for (auto& x : lowpass_) { x.reset(); }

  for (auto& x : halfbandInput_) { x.fill({}); }
  for (auto& x : halfbandIir_) { x.reset(); }

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() {
  unsigned newOverSampling = unsigned(param.value.oversampling->load());
  if (overSampling_ != newOverSampling) {
    overSampling_ = newOverSampling;
    updateUpRate();
  }
  ASSIGN_PARAMETER(push);
}

std::array<double, 2> DSPCore::processSample(const std::array<double, 2> in) {
  double sig0 = in[0];
  double sig1 = in[1];

  crossoverCutoff_.process();
  auto split0 = splitter_[0].process(sig0, crossoverCutoff_.value());
  auto split1 = splitter_[1].process(sig1, crossoverCutoff_.value());

  shaperDecaySample_.process();
  shaperRefreshRatio_.process();
  double env0 = envelopeHigh_[0].process(split0.high, shaperDecaySample_.value(),
                                         shaperRefreshRatio_.value());
  double env1 = envelopeHigh_[1].process(split1.high, shaperDecaySample_.value(),
                                         shaperRefreshRatio_.value());

  shaperPostLowpassCutoff_.process();
  env0 = lowpass_[0].process(env0, shaperPostLowpassCutoff_.value());
  env1 = lowpass_[1].process(env1, shaperPostLowpassCutoff_.value());

  modEnvelopeOutMax_[0] = std::max(modEnvelopeOutMax_[0], float(env0));
  modEnvelopeOutMax_[1] = std::max(modEnvelopeOutMax_[1], float(env1));

  shaperIntensity_.process();
  split0.high *= double(1) + shaperIntensity_.value() * env0;
  split1.high *= double(1) + shaperIntensity_.value() * env1;

  lowGain_.process();
  highGain_.process();
  return {
    lowGain_.value() * split0.low + highGain_.value() * split0.high,
    lowGain_.value() * split1.low + highGain_.value() * split1.high,
  };
};

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  auto& pv = param.value;

  inputPeakMax_.fill(0);
  modEnvelopeOutMax_.fill(0);

  std::array<double, 2> frame{};
  for (size_t i = 0; i < length; ++i) {
    inputPeakMax_[0] = std::max(inputPeakMax_[0], std::abs(in0[i]));
    inputPeakMax_[1] = std::max(inputPeakMax_[1], std::abs(in1[i]));

    const auto inSig0 = double(in0[i]);
    const auto inSig1 = double(in1[i]);

    if (overSampling_ == 1) { // 2x sampling.
      frame = processSample({
        double(0.5) * (halfbandInput_[0][0] + inSig0),
        double(0.5) * (halfbandInput_[0][1] + inSig1),
      });
      halfbandInput_[0][0] = frame[0];
      halfbandInput_[1][0] = frame[1];

      frame = processSample({inSig0, inSig1});
      halfbandInput_[0][1] = frame[0];
      halfbandInput_[1][1] = frame[1];

      frame[0] = halfbandIir_[0].process(halfbandInput_[0]);
      frame[1] = halfbandIir_[1].process(halfbandInput_[1]);
    } else { // 1x sampling.
      frame = processSample({inSig0, inSig1});
    }

    out0[i] = float(frame[0]);
    out1[i] = float(frame[1]);

    halfbandInput_[0][0] = inSig0;
    halfbandInput_[0][1] = inSig1;
  }

  for (size_t ch = 0; ch < nChannel; ++ch) {
    pv.inputPeakMax[ch] = inputPeakMax_[ch];
    pv.modEnvelopeOutMax[ch] = modEnvelopeOutMax_[ch];
  }
}

} // namespace Uhhyou
