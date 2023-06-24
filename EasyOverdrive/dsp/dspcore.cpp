// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

constexpr double limiterAttackSecond = 0.001;

template<typename T> T getKp(T sampleRate, T cutoffHz, T maxHz)
{
  return cutoffHz >= maxHz ? T(1) : T(EMAFilter<double>::cutoffToP(sampleRate, cutoffHz));
}

void DSPCore::setup(double sampleRate_)
{
  sampleRate = double(sampleRate_);

  auto maxRate = upFold * sampleRate;
  for (auto &x : overDrive) {
    x.resize(size_t(maxRate * param.scale.overDriveHoldSecond.getMax()) + 1);
  }
  for (auto &x : limiter) x.resize(size_t(maxRate * limiterAttackSecond) + 1);

  reset();
  startup();
}

size_t DSPCore::getLatency()
{
  auto &pv = param.value;

  size_t latency = 2; // Fixed 2 samples from CubicUpSampler.

  if (pv.limiterEnabled->load() != 0) {
    latency += limiter[0].latency(fold[size_t(pv.oversampling->load())]);
  }

  return latency;
}

#define ASSIGN_PARAMETER(METHOD)                                                         \
  auto &pv = param.value;                                                                \
                                                                                         \
  SmootherCommon<double>::setTime(pv.parameterSmoothingSecond->load());                  \
                                                                                         \
  overDriveType = size_t(pv.overDriveType->load());                                      \
  asymDriveEnabled = (pv.asymDriveEnabled->load()) != 0;                                 \
  limiterEnabled = (pv.limiterEnabled->load()) != 0;                                     \
                                                                                         \
  preDriveGain.METHOD(pv.preDriveGain->load());                                          \
  postDriveGain.METHOD(pv.postDriveGain->load());                                        \
  limiterInputGain.METHOD(pv.limiterInputGain->load());                                  \
                                                                                         \
  for (auto &x : overDrive) {                                                            \
    x.METHOD(                                                                            \
      upRate, pv.overDriveHoldSecond->load(), pv.overDriveQ->load(),                     \
      pv.overDriveCharacterAmp->load());                                                 \
  }                                                                                      \
                                                                                         \
  for (auto &x : asymDrive) {                                                            \
    x.METHOD(                                                                            \
      upRate, pv.asymDriveDecaySecond->load(), pv.asymDriveDecayBias->load(),            \
      pv.asymDriveQ->load(), pv.asymExponentRange->load());                              \
  }                                                                                      \
                                                                                         \
  for (auto &x : limiter) {                                                              \
    x.prepare(upRate, limiterAttackSecond, pv.limiterReleaseSecond->load(), double(1));  \
  }

void DSPCore::updateUpRate()
{
  upRate = double(sampleRate) * fold[oversampling];
  SmootherCommon<double>::setSampleRate(upRate);
}

void DSPCore::reset()
{
  oversampling = size_t(param.value.oversampling->load());
  updateUpRate();

  ASSIGN_PARAMETER(reset);

  for (auto &x : limiter) x.reset();
  for (auto &x : upSampler) x.reset();
  for (auto &x : decimationLowpass) x.reset();
  for (auto &x : halfbandIir) x.reset();

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters()
{
  size_t newOversampling = size_t(param.value.oversampling->load());
  if (oversampling != newOversampling) {
    oversampling = newOversampling;
    updateUpRate();
  }

  ASSIGN_PARAMETER(push);
}

std::array<double, 2> DSPCore::processFrame(const std::array<double, 2> &frame)
{
  preDriveGain.process();
  limiterInputGain.process();
  postDriveGain.process();

  auto sig0 = preDriveGain.v() * frame[0];
  auto sig1 = preDriveGain.v() * frame[1];

  switch (overDriveType) {
    default:
    case 0: {
      sig0 = overDrive[0].processImmediate(sig0);
      sig1 = overDrive[1].processImmediate(sig1);
    } break;

    case 1: {
      sig0 = overDrive[0].processMatched(sig0);
      sig1 = overDrive[1].processMatched(sig1);
    } break;

    case 2: {
      sig0 = overDrive[0].processBadLimiter(sig0);
      sig1 = overDrive[1].processBadLimiter(sig1);
    } break;

    case 3: {
      sig0 = overDrive[0].processPolyDrive(sig0);
      sig1 = overDrive[1].processPolyDrive(sig1);
    } break;
  }

  if (asymDriveEnabled) {
    sig0 = asymDrive[0].process(sig0);
    sig1 = asymDrive[1].process(sig1);
  }

  if (limiterEnabled) {
    sig0 = limiter[0].process(sig0 * limiterInputGain.v());
    sig1 = limiter[1].process(sig1 * limiterInputGain.v());
  }

  sig0 *= postDriveGain.v();
  sig1 *= postDriveGain.v();

  return {sig0, sig1};
}

void DSPCore::process(
  const size_t length, const float *in0, const float *in1, float *out0, float *out1)
{
  // auto &pv = param.value; // TODO

  SmootherCommon<double>::setBufferSize(double(length));

  for (size_t i = 0; i < length; ++i) {
    upSampler[0].process(in0[i]);
    upSampler[1].process(in1[i]);

    if (oversampling == 2) { // 16x.
      for (size_t j = 0; j < upFold; ++j) {
        auto frame = processFrame({upSampler[0].output[j], upSampler[1].output[j]});
        decimationLowpass[0].push(frame[0]);
        decimationLowpass[1].push(frame[1]);
        upSampler[0].output[j] = decimationLowpass[0].output();
        upSampler[1].output[j] = decimationLowpass[1].output();
      }
      out0[i] = float(halfbandIir[0].process(
        {upSampler[0].output[0], upSampler[0].output[upFold / 2]}));
      out1[i] = float(halfbandIir[1].process(
        {upSampler[1].output[0], upSampler[1].output[upFold / 2]}));
    } else if (oversampling == 1) { // 2x.
      const size_t mid = upFold / 2;
      for (size_t j = 0; j < upFold; j += mid) {
        auto frame = processFrame({upSampler[0].output[j], upSampler[1].output[j]});
        upSampler[0].output[j] = frame[0];
        upSampler[1].output[j] = frame[1];
      }
      out0[i] = float(
        halfbandIir[0].process({upSampler[0].output[0], upSampler[0].output[mid]}));
      out1[i] = float(
        halfbandIir[1].process({upSampler[1].output[0], upSampler[1].output[mid]}));
    } else { // 1x.
      auto frame = processFrame({upSampler[0].output[0], upSampler[1].output[0]});
      out0[i] = float(frame[0]);
      out1[i] = float(frame[1]);
    }
  }
}

} // namespace Uhhyou
