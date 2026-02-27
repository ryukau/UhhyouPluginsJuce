// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "dspcore.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>

namespace Uhhyou {

void DSPCore::setup(Real sampleRate_) {
  sampleRate = Real(sampleRate_);
  upRate = sampleRate * upFold;

  smoo.setTime(upRate, smootherTimeInSecond);

  const Real maxDelayTimeSeconds = Real(0.001) * param.scale.delayTimeMs.getMax();
  for (auto& x : fdn) { x.setup(upRate * maxDelayTimeSeconds); }

  reset();
  startup();
}

void DSPCore::updateUpRate() {
  upRate = sampleRate * (overSampling ? 2 : 1);
  smoo.setTime(upRate, smootherTimeInSecond);
  lfo.setSyncRate(secondToEmaAlpha(upRate, Real(0.002)));
  for (auto& x : fdn) { x.updateSamplingRate(upRate); }
  for (auto& x : halfbandIir) { x.reset(); }
  fadeKp = cutoffToEmaAlpha<Real>(Real(2) / upRate);
  noteKp = cutoffToEmaAlpha<Real>(Real(500) / upRate);
}

size_t DSPCore::getLatency() { return 0; }

template<typename Func> void DSPCore::applyToParameters(Func apply) {
  constexpr auto eps = std::numeric_limits<Real>::epsilon();
  auto& pv = param.value;
  auto& scl = param.scale;

  // L for load.
  auto L
    = [](const std::atomic<float>* const value) { return value->load(std::memory_order_relaxed); };

  noteReceive = L(pv.noteReceive) >= Real(0.5);
  notePitchScalar = -L(pv.notePitchRange);
  noteGainScalar = L(pv.noteGainRange);

  useFeedbackGate = L(pv.feedbackGate) >= Real(0.5);
  const auto newSaturatorType = static_cast<Saturator<Real>::Function>(L(pv.saturationType) + 0.5f);
  if (saturatorType != newSaturatorType) {
    for (auto& x : fdn) { x.softReset(); }
  }
  saturatorType = newSaturatorType;

  const auto satGain = L(pv.saturationGain);
  apply(saturationGain, satGain);
  apply(inputRatio, L(pv.inputRatio));
  apply(feedback0, L(pv.feedback0));
  apply(feedback1, L(pv.feedback1));
  apply(lfoPhaseInitial, L(pv.lfoPhaseInitial));
  apply(lfoPhaseStereoOffset, L(pv.lfoPhaseStereoOffset));

  const auto delayTime = L(pv.delayTimeMs) * upRate / Real(1000);
  apply(delayTimeSample0, delayTime);
  apply(delayTimeSample1, L(pv.delayTimeRatio) * delayTime);

  apply(viscosityCutoff, L(pv.viscosityLowpassHz) / upRate);
  apply(crossModMode, L(pv.crossModMode));

  const auto setMod = [&](Real invTime, Real mod, Real tracking) -> Real {
    constexpr auto invLn2 = Real(1) / std::numbers::ln2_v<Real>;
    const auto weakBound = std::log1p(std::abs(mod) * invTime) * invLn2;
    const auto scaled = std::lerp(weakBound, mod, tracking);
    return std::copysign(scaled, mod);
  };
  const auto weakScalar = upRate * Real(0.01);
  const auto invTime0 = weakScalar / std::max(Real(1), delayTimeSample0.target());
  const auto invTime1 = weakScalar / std::max(Real(1), delayTimeSample1.target());
  const auto modTracking = L(pv.modulationTracking);
  const auto modAdjust = (satGain >= Real(1)) ? Real(1) : satGain;
  const auto cMod0 = L(pv.crossModulationOctave0) / modAdjust;
  const auto cMod1 = L(pv.crossModulationOctave1) / modAdjust;
  apply(crossModOctave0, setMod(invTime0, cMod0, modTracking));
  apply(crossModOctave1, setMod(invTime1, cMod1, modTracking));
  apply(timeModOctave0, setMod(invTime0, L(pv.lfoToDelayTimeOctave0), modTracking));
  apply(timeModOctave1, setMod(invTime1, L(pv.lfoToDelayTimeOctave1), modTracking));
  apply(am0, L(pv.am0));
  apply(am1, L(pv.am1));

  const auto& cutoffMax = scl.cutoffHz.getMax();
  apply(lowpassCutoff, L(pv.lowpassCutoffHz) / upRate);
  apply(lowpassFade, (L(pv.lowpassCutoffHz) >= cutoffMax) ? Real(0) : Real(1));
  apply(highpassCutoff, L(pv.highpassCutoffHz) / upRate);
  apply(highpassFade, (L(pv.highpassCutoffHz) <= 0) ? Real(0) : Real(1));

  const auto flange = L(pv.flangeMode);
  apply(flangeMode, flange);
  apply(safeFeedback, L(pv.safeFeedback) * flange);
  apply(safeFlange, L(pv.safeFlange) < Real(0.5) ? -flange : flange);

  apply(dryGain, L(pv.dryGain));
  const auto wetSign = L(pv.wetInvert) ? Real(-1) : Real(1);
  apply(wetGain, std::copysign(L(pv.wetGain), wetSign));

  const auto upper = Real(1);
  const auto lower = Real(1);
  const auto lfoBeat = std::max(Real(L(pv.lfoBeat)), eps);
  const auto lfoRate = lfoBeat >= scl.lfoBeat.getMax() ? 0 : Real(1) / lfoBeat;
  lfo.update(tempo, lfoRate, upper, lower, upRate);

  isResettingLfoPhase = L(pv.lfoPhaseReset) >= Real(0.5);
}

void DSPCore::reset() {
  updateUpRate();

  applyToParameters([](auto& target, auto value) { target.reset(value); });

  noteIdStack.clear();
  notePitch.reset(Real(1));
  noteGain.reset(Real(1));
  globalPitchBend.reset(Real(1));

  modPhase.fill({});
  lfo.reset();

  for (auto& x : fdn) { x.reset(); }
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

  applyToParameters([](auto& target, auto value) { target.push(value); });

  // Prepare for this cycle.
  preSaturationPeak.fill({});
  outputPeak.fill({});

  for (size_t i = 0; i < nChannel; ++i) {
    for (size_t j = 0; j < 2; ++j) {
      displayTime[i].upper[j] = Real(0);
      displayTime[i].lower[j] = std::numeric_limits<Real>::max();
    }
  }
}

auto DSPCore::processSample(const std::array<Real, 2> in) -> std::array<Real, 2> {
  saturationGain.process();
  constexpr auto gateThresholdBase = Real{0.05};
  const auto gateThresholdAdjusted = useFeedbackGate
    ? (saturationGain.value() >= Real(1) ? gateThresholdBase
                                         : gateThresholdBase * saturationGain.value())
    : Real(0);

  modPhase[0] = lfo.process(isPlaying, isResettingLfoPhase, lfoPhaseInitial.process(), upRate,
                            beatsElapsed, tempo);
  modPhase[1] = modPhase[0] + lfoPhaseStereoOffset.process();
  modPhase[1] -= std::floor(modPhase[1]);

  const auto ntPitch = notePitch.process(noteKp) * globalPitchBend.process(noteKp);
  const Fdn2<Real>::Parameters params = {
    .feedbackGateThreshold = gateThresholdAdjusted,
    .feedback0 = feedback0.process(),
    .feedback1 = feedback1.process(),
    .saturatorType = saturatorType,
    .inputRatio = inputRatio.process(),
    .timeInSamples0 = delayTimeSample0.process() * ntPitch,
    .timeInSamples1 = delayTimeSample1.process() * ntPitch,
    .viscosityCutoff = viscosityCutoff.process(),
    .crossModMode = crossModMode.process(),
    .crossModOctave0 = crossModOctave0.process(),
    .crossModOctave1 = crossModOctave1.process(),
    .timeModOctave0 = timeModOctave0.process(),
    .timeModOctave1 = timeModOctave1.process(),
    .am0 = am0.process(),
    .am1 = am1.process(),
    .highpassCutoff = highpassCutoff.process(),
    .highpassFade = highpassFade.process(fadeKp),
    .flangeMode = flangeMode.process(),
    .safeFeedback = safeFeedback.process(),
    .flangeSign = safeFlange.process(),
    .lowpassCutoff = lowpassCutoff.process(),
    .lowpassFade = lowpassFade.process(fadeKp),
  };

  const auto gain = noteGain.process(noteKp) * saturationGain.value();
  auto sig0 = gain * in[0];
  auto sig1 = gain * in[1];

  preSaturationPeak[0] = std::max(std::abs(sig0), preSaturationPeak[0]);
  preSaturationPeak[1] = std::max(std::abs(sig1), preSaturationPeak[1]);

  sig0 = fdn[0].process(sig0, modPhase[0], displayTime[0], params);
  sig1 = fdn[1].process(sig1, modPhase[1], displayTime[1], params);

  if (saturationGain.value() < Real(1)) {
    constexpr auto eps = std::numeric_limits<Real>::epsilon();
    const auto& g = saturationGain.value();
    const auto cleanUpGain = std::copysign(std::max(std::abs(g), eps), g);
    sig0 /= cleanUpGain;
    sig1 /= cleanUpGain;
  }

  dryGain.process();
  wetGain.process();

  sig0 = dryGain.value() * in[0] + wetGain.value() * sig0;
  sig1 = dryGain.value() * in[1] + wetGain.value() * sig1;

  outputPeak[0] = std::max(std::abs(sig0), outputPeak[0]);
  outputPeak[1] = std::max(std::abs(sig1), outputPeak[1]);

  return {sig0, sig1};
};

void DSPCore::process(const size_t length, const float* in0, const float* in1, float* out0,
                      float* out1) {
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

  // Send values to GUI.
  constexpr auto mem = std::memory_order_relaxed;
  auto& pv = param.value;

  auto storeMax = [&](std::atomic<float>& target, Real value) {
    float candidate = float(value);
    float current = target.load(mem);
    if (current < candidate) { target.store(candidate, mem); }
  };

  storeMax(pv.displayPreSaturationPeak[0], preSaturationPeak[0]);
  storeMax(pv.displayPreSaturationPeak[1], preSaturationPeak[1]);
  storeMax(pv.displayOutputPeak[0], outputPeak[0]);
  storeMax(pv.displayOutputPeak[1], outputPeak[1]);
  pv.displayLfoPhase[0].store(float(modPhase[0]), mem);
  pv.displayLfoPhase[1].store(float(modPhase[1]), mem);

  const Real invUpRate = Real(1) / upRate;
  pv.displayDelayTimeUpper[0][0].store(float(displayTime[0].upper[0] * invUpRate), mem);
  pv.displayDelayTimeUpper[0][1].store(float(displayTime[0].upper[1] * invUpRate), mem);
  pv.displayDelayTimeLower[0][0].store(float(displayTime[0].lower[0] * invUpRate), mem);
  pv.displayDelayTimeLower[0][1].store(float(displayTime[0].lower[1] * invUpRate), mem);
  pv.displayDelayTimeUpper[1][0].store(float(displayTime[1].upper[0] * invUpRate), mem);
  pv.displayDelayTimeUpper[1][1].store(float(displayTime[1].upper[1] * invUpRate), mem);
  pv.displayDelayTimeLower[1][0].store(float(displayTime[1].lower[0] * invUpRate), mem);
  pv.displayDelayTimeLower[1][1].store(float(displayTime[1].lower[1] * invUpRate), mem);
}

template<typename Real> inline Real semitoneToRatio(Real scaler, Real semitone) {
  return std::exp2(scaler * (semitone / Real(12) - Real(5)));
}

void DSPCore::noteOn(int noteId, Real pitchSemitone, Real velocity) {
  if (!noteReceive) { return; }

  noteIdStack.push_back({
    .noteId = noteId,
    .pitch = semitoneToRatio(notePitchScalar, pitchSemitone),
    .gain = ScaleTools::dbToAmp(noteGainScalar * Real(velocity)),
  });
  notePitch.push(noteIdStack.back().pitch);
  noteGain.push(noteIdStack.back().gain);
}

void DSPCore::noteOff(int noteId) {
  auto reset = [&]() {
    notePitch.push(Real(1));
    noteGain.push(Real(1));
  };

  if (noteIdStack.empty()) [[unlikely]] {
    reset();
    return;
  }

  auto it = std::ranges::find(noteIdStack, noteId, &NoteData::noteId);
  if (it == noteIdStack.end()) { return; }

  const bool latestNoteIsOff = (it == std::prev(noteIdStack.end()));
  noteIdStack.erase(it);
  if (!latestNoteIsOff) { return; }

  if (noteIdStack.empty()) {
    reset();
  } else {
    notePitch.push(noteIdStack.back().pitch);
    noteGain.push(noteIdStack.back().gain);
  }
}

void DSPCore::notePitchBend(int noteId, Real pitchSemitone) {
  if (!noteReceive || noteIdStack.empty()) { return; }

  auto it = std::find_if(noteIdStack.rbegin(), noteIdStack.rend(),
                         [noteId](const auto& data) { return data.noteId == noteId; });

  if (it != noteIdStack.rend()) {
    it->pitch = semitoneToRatio(notePitchScalar, pitchSemitone);
    if (it == noteIdStack.rbegin()) { notePitch.push(it->pitch); }
  }
}

void DSPCore::setPitchBend(Real bend) { globalPitchBend.push(std::exp2(bend)); }

} // namespace Uhhyou
