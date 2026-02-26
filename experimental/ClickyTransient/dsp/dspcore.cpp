// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

void DSPCore::setup(double sampleRate_) {
  sampleRate = double(sampleRate_);
  upRate = sampleRate * upFold;

  smoo.setTime(upRate, smootherTimeInSecond);

  reset();
  startup();
}

void DSPCore::updateUpRate() {
  upRate = sampleRate * (overSampling ? 2 : 1);
  smoo.setTime(upRate, smootherTimeInSecond);
}

size_t DSPCore::getLatency() { return splitter[0].latency; }

#define ASSIGN_PARAMETER(METHOD)                                                                   \
  auto& pv = param.value;                                                                          \
                                                                                                   \
  crossoverCutoff.METHOD(pv.crossoverHz->load() / upRate);                                         \
  shaperDecaySample.METHOD(pv.shaperDecaySecond->load() * upRate);                                 \
  shaperRefreshRatio.METHOD(pv.shaperRefreshRatio->load());                                        \
  const auto intensity = pv.shaperIntensity->load();                                               \
  shaperIntensity.METHOD(intensity);                                                               \
  shaperPostLowpassCutoff.METHOD(pv.shaperPostLowpassHz->load() / upRate);                         \
  lowGain.METHOD(pv.lowGain->load());                                                              \
  highGain.METHOD(pv.highGain->load() / std::sqrt(double(1) + intensity));

void DSPCore::reset() {
  updateUpRate();

  ASSIGN_PARAMETER(reset);

  for (auto& x : pv.inputPeakMax) { x = float(0); }
  for (auto& x : pv.modEnvelopeOutMax) { x = float(0); }

  for (auto& x : splitter) { x.reset(); }
  for (auto& x : envelopeHigh) { x.reset(); }
  for (auto& x : lowpass) { x.reset(); }

  for (auto& x : halfbandInput) { x.fill({}); }
  for (auto& x : halfbandIir) { x.reset(); }

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() {
  unsigned newOverSampling = unsigned(param.value.oversampling->load());
  if (overSampling != newOverSampling) {
    overSampling = newOverSampling;
    updateUpRate();
  }
  ASSIGN_PARAMETER(push);
}

std::array<double, 2> DSPCore::processSample(const std::array<double, 2> in) {
  double sig0 = in[0];
  double sig1 = in[1];

  crossoverCutoff.process();
  auto split0 = splitter[0].process(sig0, crossoverCutoff.value());
  auto split1 = splitter[1].process(sig1, crossoverCutoff.value());

  shaperDecaySample.process();
  shaperRefreshRatio.process();
  double env0
    = envelopeHigh[0].process(split0.high, shaperDecaySample.value(), shaperRefreshRatio.value());
  double env1
    = envelopeHigh[1].process(split1.high, shaperDecaySample.value(), shaperRefreshRatio.value());

  shaperPostLowpassCutoff.process();
  env0 = lowpass[0].process(env0, shaperPostLowpassCutoff.value());
  env1 = lowpass[1].process(env1, shaperPostLowpassCutoff.value());

  modEnvelopeOutMax[0] = std::max(modEnvelopeOutMax[0], float(env0));
  modEnvelopeOutMax[1] = std::max(modEnvelopeOutMax[1], float(env1));

  shaperIntensity.process();
  split0.high *= double(1) + shaperIntensity.value() * env0;
  split1.high *= double(1) + shaperIntensity.value() * env1;

  lowGain.process();
  highGain.process();
  return {
    lowGain.value() * split0.low + highGain.value() * split0.high,
    lowGain.value() * split1.low + highGain.value() * split1.high,
  };
};

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  auto& pv = param.value;

  inputPeakMax.fill(0);
  modEnvelopeOutMax.fill(0);

  std::array<double, 2> frame{};
  for (size_t i = 0; i < length; ++i) {
    inputPeakMax[0] = std::max(inputPeakMax[0], std::abs(in0[i]));
    inputPeakMax[1] = std::max(inputPeakMax[1], std::abs(in1[i]));

    const auto inSig0 = double(in0[i]);
    const auto inSig1 = double(in1[i]);

    if (overSampling == 1) { // 2x sampling.
      frame = processSample({
        double(0.5) * (halfbandInput[0][0] + inSig0),
        double(0.5) * (halfbandInput[0][1] + inSig1),
      });
      halfbandInput[0][0] = frame[0];
      halfbandInput[1][0] = frame[1];

      frame = processSample({inSig0, inSig1});
      halfbandInput[0][1] = frame[0];
      halfbandInput[1][1] = frame[1];

      frame[0] = halfbandIir[0].process(halfbandInput[0]);
      frame[1] = halfbandIir[1].process(halfbandInput[1]);
    } else { // 1x sampling.
      frame = processSample({inSig0, inSig1});
    }

    out0[i] = float(frame[0]);
    out1[i] = float(frame[1]);

    halfbandInput[0][0] = inSig0;
    halfbandInput[0][1] = inSig1;
  }

  for (size_t ch = 0; ch < nChannel; ++ch) {
    pv.inputPeakMax[ch] = inputPeakMax[ch];
    pv.modEnvelopeOutMax[ch] = modEnvelopeOutMax[ch];
  }
}

} // namespace Uhhyou
