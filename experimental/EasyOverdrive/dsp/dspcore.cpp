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

void DSPCore::setup(double sampleRate) {
  sampleRate_ = double(sampleRate);

  auto maxRate = upFold * sampleRate_;
  for (auto& x : overDrive_) {
    x.resize(size_t(maxRate * param.scale.overDriveHoldSecond.getMax()) + 1);
  }
  for (auto& x : limiter_) { x.resize(size_t(maxRate * limiterAttackSecond) + 1); }

  reset();
  startup();
}

size_t DSPCore::getLatency() {
  auto& pv = param.value;

  size_t latency = 2; // Fixed 2 samples from CubicUpSampler.

  if (pv.limiterEnabled->load() != 0) {
    latency += limiter_[0].latency(fold[size_t(pv.oversampling->load())]);
  }

  return latency;
}

#define ASSIGN_PARAMETER(METHOD)                                                                   \
  auto& pv = param.value;                                                                          \
                                                                                                   \
  smoo_.setTime(upRate_, pv.parameterSmoothingSecond->load());                                     \
                                                                                                   \
  overDriveType_ = size_t(pv.overDriveType->load());                                               \
  asymDriveEnabled_ = (pv.asymDriveEnabled->load()) != 0;                                          \
  limiterEnabled_ = (pv.limiterEnabled->load()) != 0;                                              \
                                                                                                   \
  preDriveGain_.METHOD(pv.preDriveGain->load());                                                   \
  postDriveGain_.METHOD(pv.postDriveGain->load());                                                 \
  limiterInputGain_.METHOD(pv.limiterInputGain->load());                                           \
                                                                                                   \
  for (auto& x : overDrive_) {                                                                     \
    x.METHOD(upRate_, pv.overDriveHoldSecond->load(), pv.overDriveQ->load(),                       \
             pv.overDriveCharacterAmp->load());                                                    \
  }                                                                                                \
                                                                                                   \
  for (auto& x : asymDrive_) {                                                                     \
    x.METHOD(upRate_, pv.asymDriveDecaySecond->load(), pv.asymDriveDecayBias->load(),              \
             pv.asymDriveQ->load(), pv.asymExponentRange->load());                                 \
  }                                                                                                \
                                                                                                   \
  for (auto& x : limiter_) {                                                                       \
    x.prepare(upRate_, limiterAttackSecond, pv.limiterReleaseSecond->load(), double(1));           \
  }

void DSPCore::updateUpRate() { upRate_ = double(sampleRate_) * fold[oversampling_]; }

void DSPCore::reset() {
  oversampling_ = size_t(param.value.oversampling->load());
  updateUpRate();

  ASSIGN_PARAMETER(reset);

  for (auto& x : limiter_) { x.reset(); }
  for (auto& x : upSampler_) { x.reset(); }
  for (auto& x : decimationLowpass_) { x.reset(); }
  for (auto& x : halfbandIir_) { x.reset(); }

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters() {
  size_t newOversampling = size_t(param.value.oversampling->load());
  if (oversampling_ != newOversampling) {
    oversampling_ = newOversampling;
    updateUpRate();
  }

  ASSIGN_PARAMETER(push);
}

std::array<double, 2> DSPCore::processFrame(const std::array<double, 2>& frame) {
  preDriveGain_.process();
  limiterInputGain_.process();
  postDriveGain_.process();

  auto sig0 = preDriveGain_.value() * frame[0];
  auto sig1 = preDriveGain_.value() * frame[1];

  switch (overDriveType_) {
    default:
    case BadLimiterType::Immediate: {
      sig0 = overDrive_[0].processImmediate(sig0);
      sig1 = overDrive_[1].processImmediate(sig1);
    } break;

    case BadLimiterType::HardGate: {
      sig0 = overDrive_[0].processHardGate(sig0);
      sig1 = overDrive_[1].processHardGate(sig1);
    } break;

    case BadLimiterType::Spike: {
      sig0 = overDrive_[0].processSpike(sig0);
      sig1 = overDrive_[1].processSpike(sig1);
    } break;

    case BadLimiterType::SpikeCubic: {
      sig0 = overDrive_[0].processSpikeCubic(sig0);
      sig1 = overDrive_[1].processSpikeCubic(sig1);
    } break;

    case BadLimiterType::CutoffMod: {
      sig0 = overDrive_[0].processCutoffMod(sig0);
      sig1 = overDrive_[1].processCutoffMod(sig1);
    } break;

    case BadLimiterType::Matched: {
      sig0 = overDrive_[0].processMatched(sig0);
      sig1 = overDrive_[1].processMatched(sig1);
    } break;

    case BadLimiterType::BadLimiter: {
      sig0 = overDrive_[0].processBadLimiter(sig0);
      sig1 = overDrive_[1].processBadLimiter(sig1);
    } break;

    case BadLimiterType::PolyDrive: {
      sig0 = overDrive_[0].processPolyDrive(sig0);
      sig1 = overDrive_[1].processPolyDrive(sig1);
    } break;
  }

  if (asymDriveEnabled_) {
    sig0 = asymDrive_[0].process(sig0);
    sig1 = asymDrive_[1].process(sig1);
  }

  if (limiterEnabled_) {
    sig0 = limiter_[0].process(sig0 * limiterInputGain_.value());
    sig1 = limiter_[1].process(sig1 * limiterInputGain_.value());
  }

  sig0 *= postDriveGain_.value();
  sig1 *= postDriveGain_.value();

  return {sig0, sig1};
}

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  for (size_t i = 0; i < length; ++i) {
    upSampler_[0].process(in0[i]);
    upSampler_[1].process(in1[i]);

    if (oversampling_ == 2) { // 16x.
      for (size_t j = 0; j < upFold; ++j) {
        auto frame = processFrame({upSampler_[0].output[j], upSampler_[1].output[j]});
        decimationLowpass_[0].push(frame[0]);
        decimationLowpass_[1].push(frame[1]);
        upSampler_[0].output[j] = decimationLowpass_[0].output();
        upSampler_[1].output[j] = decimationLowpass_[1].output();
      }
      out0[i] = float(
        halfbandIir_[0].process({upSampler_[0].output[0], upSampler_[0].output[upFold / 2]}));
      out1[i] = float(
        halfbandIir_[1].process({upSampler_[1].output[0], upSampler_[1].output[upFold / 2]}));
    } else if (oversampling_ == 1) { // 2x.
      const size_t mid = upFold / 2;
      for (size_t j = 0; j < upFold; j += mid) {
        auto frame = processFrame({upSampler_[0].output[j], upSampler_[1].output[j]});
        upSampler_[0].output[j] = frame[0];
        upSampler_[1].output[j] = frame[1];
      }
      out0[i]
        = float(halfbandIir_[0].process({upSampler_[0].output[0], upSampler_[0].output[mid]}));
      out1[i]
        = float(halfbandIir_[1].process({upSampler_[1].output[0], upSampler_[1].output[mid]}));
    } else { // 1x.
      auto frame = processFrame({upSampler_[0].output[0], upSampler_[1].output[0]});
      out0[i] = float(frame[0]);
      out1[i] = float(frame[1]);
    }
  }
}

} // namespace Uhhyou
