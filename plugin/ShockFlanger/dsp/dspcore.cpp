// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

void DSPCore::setup(Real sampleRate) {
  sampleRate_ = Real(sampleRate);
  upRate_ = sampleRate_ * upFold;

  smoo_.setTime(upRate_, smootherTimeInSecond);

  const Real maxDelayTimeSeconds = Real(0.001) * param.scale.delayTimeMs.getMax();
  for (auto& x : fdn_) { x.setup(upRate_ * maxDelayTimeSeconds); }

  reset();
  startup();
}

void DSPCore::updateUpRate() {
  upRate_ = sampleRate_ * (overSampling_ ? 2 : 1);
  smoo_.setTime(upRate_, smootherTimeInSecond);
  lfo_.setSyncRate(secondToEmaAlpha(upRate_, Real(0.002)));
  for (auto& x : fdn_) { x.updateSamplingRate(upRate_); }
  for (auto& x : halfbandIir_) { x.reset(); }
  fadeKp_ = cutoffToEmaAlpha<Real>(Real(2) / upRate_);
  noteKp_ = cutoffToEmaAlpha<Real>(Real(500) / upRate_);
}

size_t DSPCore::getLatency() { return 0; }

template<typename Func> void DSPCore::applyToParameters(Func apply) {
  constexpr auto eps = std::numeric_limits<Real>::epsilon();
  auto& pv = param.value;
  auto& scl = param.scale;

  // L for load.
  auto L
    = [](const std::atomic<float>* const value) { return value->load(std::memory_order_relaxed); };

  noteReceive_ = L(pv.noteReceive) >= Real(0.5);
  notePitchScalar_ = -L(pv.notePitchRange);
  noteGainScalar_ = L(pv.noteGainRange);

  useFeedbackGate_ = L(pv.feedbackGate) >= Real(0.5);
  const auto newSaturatorType = static_cast<Saturator<Real>::Function>(L(pv.saturationType) + 0.5f);
  if (saturatorType_ != newSaturatorType) {
    for (auto& x : fdn_) { x.softReset(); }
  }
  saturatorType_ = newSaturatorType;

  const auto satGain = L(pv.saturationGain);
  apply(saturationGain_, satGain);
  apply(inputBlend_, L(pv.inputBlend));
  apply(feedback0_, L(pv.feedback0));
  apply(feedback1_, L(pv.feedback1));
  apply(lfoPhaseInitial_, L(pv.lfoPhaseInitial));
  apply(lfoPhaseStereoOffset_, L(pv.lfoPhaseStereoOffset));

  const auto delayTime = L(pv.delayTimeMs) * upRate_ / Real(1000);
  apply(delayTimeSample0_, delayTime);
  apply(delayTimeSample1_, L(pv.delayTimeRatio) * delayTime);

  apply(viscosityCutoff_, L(pv.viscosityLowpassHz) / upRate_);
  apply(audioModMode_, L(pv.audioModMode));

  const auto setMod = [&](Real invTime, Real mod, Real tracking) -> Real {
    constexpr auto invLn2 = Real(1) / std::numbers::ln2_v<Real>;
    const auto weakBound = std::log1p(std::abs(mod) * invTime) * invLn2;
    const auto scaled = std::lerp(weakBound, mod, tracking);
    return std::copysign(scaled, mod);
  };
  const auto weakScalar = upRate_ * Real(0.01);
  const auto invTime0 = weakScalar / std::max(Real(1), delayTimeSample0_.target());
  const auto invTime1 = weakScalar / std::max(Real(1), delayTimeSample1_.target());
  const auto modTracking = L(pv.modulationTracking);
  const auto modAdjust = (satGain >= Real(1)) ? Real(1) : satGain;
  const auto cMod0 = L(pv.audioTimeMod0) / modAdjust;
  const auto cMod1 = L(pv.audioTimeMod1) / modAdjust;
  apply(audioTimeMod0_, setMod(invTime0, cMod0, modTracking));
  apply(audioTimeMod1_, setMod(invTime1, cMod1, modTracking));
  apply(lfoTimeMod0_, setMod(invTime0, L(pv.lfoTimeMod0), modTracking));
  apply(lfoTimeMod1_, setMod(invTime1, L(pv.lfoTimeMod1), modTracking));
  apply(audioAmpMod0_, L(pv.audioAmpMod0));
  apply(audioAmpMod1_, -L(pv.audioAmpMod1));

  const auto& cutoffMax = scl.cutoffHz.getMax();
  apply(lowpassCutoff_, L(pv.lowpassCutoffHz) / upRate_);
  apply(lowpassFade_, (L(pv.lowpassCutoffHz) >= cutoffMax) ? Real(0) : Real(1));
  apply(highpassCutoff_, L(pv.highpassCutoffHz) / upRate_);
  apply(highpassFade_, (L(pv.highpassCutoffHz) <= 0) ? Real(0) : Real(1));

  const auto flange = L(pv.flangeBlend);
  apply(flangeBlend_, flange);
  apply(safeFeedback_, L(pv.safeFeedback) * flange);
  apply(flangePolarity_, L(pv.flangePolarity));

  apply(dryGain_, L(pv.dryGain));
  const auto wetSign = static_cast<bool>(L(pv.wetInvert)) ? Real(-1) : Real(1);
  apply(wetGain_, std::copysign(L(pv.wetGain), wetSign));

  using SyncMode = TempoSyncedLfo<Real>::Synchronization;
  SyncMode mode = static_cast<SyncMode>(L(pv.lfoSyncType));
  constexpr auto upper = Real(1);
  constexpr auto lower = Real(1);
  const auto lfoBeat = std::max(Real(L(pv.lfoBeat)), eps);
  const auto lfoRate = lfoBeat >= scl.lfoBeat.getMax() ? 0 : Real(1) / lfoBeat;
  lfo_.update(mode, tempo, lfoRate, upper, lower, upRate_);

  isResettingLfoPhase_ = L(pv.lfoPhaseReset) >= Real(0.5);
}

void DSPCore::reset() {
  updateUpRate();

  applyToParameters([](auto& target, auto value) { target.reset(value); });

  noteIdStack_.clear();
  notePitch_.reset(Real(1));
  noteGain_.reset(Real(1));
  globalPitchBend_.reset(Real(1));

  modPhase_.fill({});
  lfo_.reset();

  for (auto& x : fdn_) { x.reset(); }
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

  applyToParameters([](auto& target, auto value) { target.push(value); });

  // Prepare for this cycle.
  preSaturationPeak_.fill({});
  outputPeak_.fill({});

  for (size_t i = 0; i < nChannel; ++i) {
    for (size_t j = 0; j < 2; ++j) {
      displayTime_[i].upper[j] = Real(0);
      displayTime_[i].lower[j] = std::numeric_limits<Real>::max();
    }
  }
}

auto DSPCore::processSample(const std::array<Real, 2> in) -> std::array<Real, 2> {
  saturationGain_.process();
  constexpr auto gateThresholdBase = Real{0.05};
  const auto gateThresholdAdjusted = useFeedbackGate_
    ? (saturationGain_.value() >= Real(1) ? gateThresholdBase
                                          : gateThresholdBase * saturationGain_.value())
    : Real(0);

  modPhase_[0] = lfo_.process(isPlaying, isResettingLfoPhase_, lfoPhaseInitial_.process(), upRate_,
                              beatsElapsed, tempo);
  modPhase_[1] = modPhase_[0] + lfoPhaseStereoOffset_.process();
  modPhase_[1] -= std::floor(modPhase_[1]);

  const auto ntPitch = notePitch_.process(noteKp_) * globalPitchBend_.process(noteKp_);
  const Fdn2<Real>::Parameters params = {
    .feedbackGateThreshold = gateThresholdAdjusted,
    .feedback0 = feedback0_.process(),
    .feedback1 = feedback1_.process(),
    .inputBlend = inputBlend_.process(),
    .timeInSamples0 = delayTimeSample0_.process() * ntPitch,
    .timeInSamples1 = delayTimeSample1_.process() * ntPitch,
    .viscosityCutoff = viscosityCutoff_.process(),
    .audioModMode = audioModMode_.process(),
    .audioTimeMod0 = audioTimeMod0_.process(),
    .audioTimeMod1 = audioTimeMod1_.process(),
    .lfoTimeMod0 = lfoTimeMod0_.process(),
    .lfoTimeMod1 = lfoTimeMod1_.process(),
    .audioAmpMod0 = audioAmpMod0_.process(),
    .audioAmpMod1 = audioAmpMod1_.process(),
    .highpassCutoff = highpassCutoff_.process(),
    .highpassFade = highpassFade_.process(fadeKp_),
    .flangeBlend = flangeBlend_.process(),
    .safeFeedback = safeFeedback_.process(),
    .flangeSign = flangePolarity_.process(),
    .lowpassCutoff = lowpassCutoff_.process(),
    .lowpassFade = lowpassFade_.process(fadeKp_),
    .saturatorType = saturatorType_,
  };

  const auto gain = noteGain_.process(noteKp_) * saturationGain_.value();
  auto sig0 = gain * in[0];
  auto sig1 = gain * in[1];

  preSaturationPeak_[0] = std::max(std::abs(sig0), preSaturationPeak_[0]);
  preSaturationPeak_[1] = std::max(std::abs(sig1), preSaturationPeak_[1]);

  sig0 = fdn_[0].process(sig0, modPhase_[0], displayTime_[0], params);
  sig1 = fdn_[1].process(sig1, modPhase_[1], displayTime_[1], params);

  if (saturationGain_.value() < Real(1)) {
    constexpr auto eps = std::numeric_limits<Real>::epsilon();
    const auto& g = saturationGain_.value();
    const auto cleanUpGain = std::copysign(std::max(std::abs(g), eps), g);
    sig0 /= cleanUpGain;
    sig1 /= cleanUpGain;
  }

  dryGain_.process();
  wetGain_.process();

  sig0 = dryGain_.value() * in[0] + wetGain_.value() * sig0;
  sig1 = dryGain_.value() * in[1] + wetGain_.value() * sig1;

  outputPeak_[0] = std::max(std::abs(sig0), outputPeak_[0]);
  outputPeak_[1] = std::max(std::abs(sig1), outputPeak_[1]);

  return {sig0, sig1};
}

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
  std::array<Real, 2> frame{};
  for (size_t i = 0; i < length; ++i) {
    const auto inSig0 = Real(in0[i]);
    const auto inSig1 = Real(in1[i]);

    if (overSampling_ == 1) { // 2x sampling.
      frame = processSample({
        Real(0.5) * (halfbandInput_[0][0] + inSig0),
        Real(0.5) * (halfbandInput_[0][1] + inSig1),
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

  // Send values to GUI.
  constexpr auto mem = std::memory_order_relaxed;
  auto& pv = param.value;

  auto storeMax = [&](std::atomic<float>& target, Real value) {
    float candidate = float(value);
    float current = target.load(mem);
    if (current < candidate) { target.store(candidate, mem); }
  };

  storeMax(pv.displayPreSaturationPeak[0], preSaturationPeak_[0]);
  storeMax(pv.displayPreSaturationPeak[1], preSaturationPeak_[1]);
  storeMax(pv.displayOutputPeak[0], outputPeak_[0]);
  storeMax(pv.displayOutputPeak[1], outputPeak_[1]);
  pv.displayLfoPhase[0].store(float(modPhase_[0]), mem);
  pv.displayLfoPhase[1].store(float(modPhase_[1]), mem);

  const Real invUpRate = Real(1) / upRate_;
  pv.displayDelayTimeUpper[0][0].store(float(displayTime_[0].upper[0] * invUpRate), mem);
  pv.displayDelayTimeUpper[0][1].store(float(displayTime_[0].upper[1] * invUpRate), mem);
  pv.displayDelayTimeLower[0][0].store(float(displayTime_[0].lower[0] * invUpRate), mem);
  pv.displayDelayTimeLower[0][1].store(float(displayTime_[0].lower[1] * invUpRate), mem);
  pv.displayDelayTimeUpper[1][0].store(float(displayTime_[1].upper[0] * invUpRate), mem);
  pv.displayDelayTimeUpper[1][1].store(float(displayTime_[1].upper[1] * invUpRate), mem);
  pv.displayDelayTimeLower[1][0].store(float(displayTime_[1].lower[0] * invUpRate), mem);
  pv.displayDelayTimeLower[1][1].store(float(displayTime_[1].lower[1] * invUpRate), mem);
}

template<typename Real> inline Real semitoneToRatio(Real scaler, Real semitone) {
  return std::exp2(scaler * (semitone / Real(12) - Real(5)));
}

void DSPCore::noteOn(int noteId, Real pitchSemitone, Real velocity) {
  if (!noteReceive_) { return; }

  noteIdStack_.push_back({
    .pitch = semitoneToRatio(notePitchScalar_, pitchSemitone),
    .gain = ScaleTools::dbToAmp(noteGainScalar_ * Real(velocity)),
    .noteId = noteId,
  });
  notePitch_.push(noteIdStack_.back().pitch);
  noteGain_.push(noteIdStack_.back().gain);
}

void DSPCore::noteOff(int noteId) {
  auto reset = [&]() {
    notePitch_.push(Real(1));
    noteGain_.push(Real(1));
  };

  if (noteIdStack_.empty()) [[unlikely]] {
    reset();
    return;
  }

  auto it = std::ranges::find(noteIdStack_, noteId, &NoteData::noteId);
  if (it == noteIdStack_.end()) { return; }

  const bool latestNoteIsOff = (it == std::prev(noteIdStack_.end()));
  noteIdStack_.erase(it);
  if (!latestNoteIsOff) { return; }

  if (noteIdStack_.empty()) {
    reset();
  } else {
    notePitch_.push(noteIdStack_.back().pitch);
    noteGain_.push(noteIdStack_.back().gain);
  }
}

void DSPCore::notePitchBend(int noteId, Real pitchSemitone) {
  if (!noteReceive_ || noteIdStack_.empty()) { return; }

  auto it = std::find_if(noteIdStack_.rbegin(), noteIdStack_.rend(),
                         [noteId](const auto& data) { return data.noteId == noteId; });

  if (it != noteIdStack_.rend()) {
    it->pitch = semitoneToRatio(notePitchScalar_, pitchSemitone);
    if (it == noteIdStack_.rbegin()) { notePitch_.push(it->pitch); }
  }
}

void DSPCore::setPitchBend(Real bend) { globalPitchBend_.push(std::exp2(bend)); }

} // namespace Uhhyou
