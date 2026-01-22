// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

void DSPCore::setup(Real sampleRate_)
{
  sampleRate = Real(sampleRate_);
  upRate = sampleRate * upFold;

  smoo.setTime(upRate, smootherTimeInSecond);
  for (auto &x : fdn) x.setup(upRate * param.scale.delayTimeMs.getMax() / 1000.0);

  reset();
  startup();
}

void DSPCore::updateUpRate()
{
  upRate = sampleRate * (overSampling ? 2 : 1);
  smoo.setTime(upRate, smootherTimeInSecond);
  lfo.setSyncRate(secondToEmaAlpha(upRate, Real(0.002)));
  for (auto &x : fdn) x.updateSamplingRate(upRate);
  for (auto &x : halfbandIir) x.reset();
  fadeKp = cutoffToEmaAlpha<Real>(Real(2) / upRate);
}

size_t DSPCore::getLatency() { return 0; }

template<typename Func> void DSPCore::applyToParameters(Func apply)
{
  constexpr auto pi = std::numbers::pi_v<Real>;
  constexpr auto eps = std::numeric_limits<Real>::epsilon();
  auto &pv = param.value;
  auto &scl = param.scale;

  useFeedbackGate = pv.feedbackGate->load() >= Real(0.5);
  const auto newSaturatorType
    = static_cast<Saturator<Real>::Function>(pv.saturationType->load() + 0.5f);
  if (saturatorType != newSaturatorType) {
    for (auto &x : fdn) x.softReset();
  }
  saturatorType = newSaturatorType;

  const auto satGain = pv.saturationGain->load();
  apply(saturationGain, satGain);
  apply(feedback0, pv.feedback0->load());
  apply(feedback1, pv.feedback1->load());
  apply(notchMix, pv.notchMix->load());
  apply(stereoPhaseOffset, pv.stereoPhaseOffset->load());

  apply(notchWidth, std::exp(-pv.notchWidthHz->load() * pi / upRate));
  apply(notchTracking, pv.notchTrackingHz->load() / upRate);

  const auto delayTime = pv.delayTimeMs->load() * upRate / Real(1000);
  apply(delayTimeSample0, delayTime);
  apply(delayTimeSample1, pv.delayTimeRatio->load() * delayTime);

  const auto delayTimeModBase = pv.rotationToDelayTimeOctave0->load();
  apply(timeModOctave0, delayTimeModBase);
  apply(timeModOctave1, delayTimeModBase * pv.rotationToDelayTimeOctave1->load());

  const auto modAdjust = (satGain >= Real(1)) ? Real(1) : satGain;
  apply(crossModOctave0, pv.crossModulationOctave0->load() / modAdjust);
  apply(crossModOctave1, pv.crossModulationOctave1->load() / modAdjust);

  const auto &cutoffMax = scl.cutoffHz.getMax();
  apply(lowpassCutoff, pv.lowpassCutoffHz->load() / upRate);
  apply(lowpassFade, (pv.lowpassCutoffHz->load() >= cutoffMax) ? Real(0) : Real(1));
  apply(highpassCutoff, pv.highpassCutoffHz->load() / upRate);
  apply(highpassFade, (pv.highpassCutoffHz->load() <= 0) ? Real(0) : Real(1));

  const auto flange = pv.flangeMode->load();
  apply(flangeMode, flange);
  apply(safeFeedback, pv.safeFeedback->load() * flange);
  apply(safeFlange, pv.safeFlange->load() < Real(0.5) ? -flange : flange);

  apply(dryGain, pv.dryGain->load());
  apply(wetGain, pv.wetGain->load());

  const auto upper = Real(1);
  const auto lower = Real(1);
  const auto lfoBeat = std::max(Real(pv.lfoBeat->load()), eps);
  const auto lfoRate = lfoBeat >= scl.lfoBeat.getMax() ? 0 : Real(1) / lfoBeat;
  lfo.update(tempo, lfoRate, upper, lower, upRate);
}

void DSPCore::reset()
{
  updateUpRate();

  applyToParameters([](auto &target, auto value) { target.reset(value); });

  constexpr Real lfoInitialPhase = 0; // TODO: Add a parameter for this.
  lfo.reset(lfoInitialPhase);

  for (auto &x : fdn) x.reset();
  for (auto &x : halfbandInput) x.fill({});
  for (auto &x : halfbandIir) x.reset();

  startup();
}

void DSPCore::startup() {}

void DSPCore::setParameters()
{
  unsigned newOverSampling = unsigned(param.value.oversampling->load());
  if (overSampling != newOverSampling) {
    overSampling = newOverSampling;
    updateUpRate();
  }

  applyToParameters([](auto &target, auto value) { target.push(value); });
}

auto DSPCore::processSample(const std::array<Real, 2> in) -> std::array<Real, 2>
{
  saturationGain.process();
  constexpr auto gateThresholdBase = Real{0.05};
  const auto gateThresholdAdjusted = useFeedbackGate
    ? (saturationGain.value() >= Real(1) ? gateThresholdBase
                                         : gateThresholdBase * saturationGain.value())
    : Real(0);

  constexpr Real lfoInitialPhase = 0; // TODO: Add a parameter for this.
  auto modPhase0 = lfo.process(isPlaying, upRate, beatsElapsed, tempo, lfoInitialPhase);
  auto modPhase1 = modPhase0 + stereoPhaseOffset.process();
  modPhase1 -= std::floor(modPhase1);

  const Fdn2<Real>::Parameters params = {
    .feedbackGateThreshold = gateThresholdAdjusted,
    .feedback0 = feedback0.process(),
    .feedback1 = feedback1.process(),
    .notchMix = notchMix.process(),
    .notchWidth = notchWidth.process(),
    .notchTracking = notchTracking.process(),
    .saturatorType = saturatorType,
    .timeInSamples0 = delayTimeSample0.process(),
    .timeInSamples1 = delayTimeSample1.process(),
    .timeModOctave0 = timeModOctave0.process(),
    .timeModOctave1 = timeModOctave1.process(),
    .crossModOctave0 = crossModOctave0.process(),
    .crossModOctave1 = crossModOctave1.process(),
    .highpassCutoff = highpassCutoff.process(),
    .highpassFade = highpassFade.process(fadeKp),
    .flangeMode = flangeMode.process(),
    .safeFeedback = safeFeedback.process(),
    .flangeSign = safeFlange.process(),
    .lowpassCutoff = lowpassCutoff.process(),
    .lowpassFade = lowpassFade.process(fadeKp),
  };
  auto sig0 = fdn[0].process(saturationGain.value() * in[0], modPhase0, params);
  auto sig1 = fdn[1].process(saturationGain.value() * in[1], modPhase1, params);

  if (saturationGain.value() < Real(1)) {
    constexpr auto eps = std::numeric_limits<Real>::epsilon();
    const auto &g = saturationGain.value();
    const auto cleanUpGain = std::copysign(std::max(std::abs(g), eps), g);
    sig0 /= cleanUpGain;
    sig1 /= cleanUpGain;
  }

  dryGain.process();
  wetGain.process();
  return {
    dryGain.value() * in[0] + wetGain.value() * sig0,
    dryGain.value() * in[1] + wetGain.value() * sig1,
  };
};

void DSPCore::process(
  const size_t length, const float *in0, const float *in1, float *out0, float *out1)
{
  std::array<Real, 2> frame{};
  for (size_t i = 0; i < length; ++i) {
    const auto inSig0 = Real(in0[i]);
    const auto inSig1 = Real(in1[i]);

    if (overSampling == 1) { // 2x sampling.
      frame = processSample({
        Real(0.5) * (halfbandInput[0][0] + inSig0),
        Real(0.5) * (halfbandInput[0][1] + inSig1),
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
}

} // namespace Uhhyou
