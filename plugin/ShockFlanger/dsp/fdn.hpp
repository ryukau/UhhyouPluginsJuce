// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/dsp/smoother.hpp"
#include "saturator.hpp"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <numbers>
#include <random>
#include <type_traits>
#include <vector>

namespace Uhhyou {

// Pade [7/8]. `x` should be in [0, 0.499*pi].
template<typename Real> inline Real tanForButterworth(Real x) {
  const Real x2 = x * x;
  return -((x * (((Real(36) * x2 - Real(6930)) * x2 + Real(270270)) * x2 - Real(2027025))
            / ((((x2 - Real(630)) * x2 + Real(51975)) * x2 - Real(945945)) * x2 + Real(2027025))));
}

template<typename Real, int order = 2> class ButterworthLowpass {
  static_assert(order > 0 && order % 2 == 0);

private:
  static constexpr size_t nSections = order / 2;

  static inline const std::array<Real, nSections> damping = []() {
    std::array<Real, nSections> d{};
    for (size_t i = 0; i < nSections; ++i) {
      const Real theta
        = (std::numbers::pi_v<Real> * (Real(2) * Real(i) + Real(1))) / (Real(2) * Real(order));
      d[i] = Real(2) * std::sin(theta);
    }
    return d;
  }();

  struct SectionState {
    Real s1 = 0;
    Real s2 = 0;
  };
  std::array<SectionState, nSections> states_{};

public:
  void reset() { states_.fill({}); }

  Real process(Real input, Real cutoffNormalized) {
    const Real c = std::clamp(cutoffNormalized, std::numeric_limits<Real>::epsilon(), Real(0.499));
    const Real w = std::numbers::pi_v<Real> * c;
    const Real g = tanForButterworth(w);
    const Real g2 = g * g;
    for (size_t idx = 0; idx < nSections; ++idx) {
      SectionState& s = states_[idx];
      const Real d = damping[idx];
      const Real y_hp = (input - (s.s1 * (g + d) + s.s2)) / (Real(1) + d * g + g2);
      const Real y_bp = y_hp * g + s.s1;
      const Real y_lp = y_bp * g + s.s2;
      const Real two_g = Real(2) * g;
      s.s1 += two_g * y_hp;
      s.s2 += two_g * y_bp;
      input = y_lp;
    }
    return input;
  }
};

template<typename Real, int order = 2> class ButterworthHighpass {
  static_assert(order > 0 && order % 2 == 0);

private:
  static constexpr size_t nSections = order / 2;

  static inline const std::array<Real, nSections> damping = []() {
    std::array<Real, nSections> d{};
    for (size_t i = 0; i < nSections; ++i) {
      const Real theta
        = (std::numbers::pi_v<Real> * (Real(2) * Real(i) + Real(1))) / (Real(2) * Real(order));
      d[i] = Real(2) * std::sin(theta);
    }
    return d;
  }();

  struct SectionState {
    Real s1 = 0;
    Real s2 = 0;
  };
  std::array<SectionState, nSections> states_{};

public:
  void reset() { states_.fill({}); }

  Real process(Real input, Real cutoffNormalized) {
    const Real w = std::numbers::pi_v<Real>
      * std::clamp(cutoffNormalized, std::numeric_limits<Real>::epsilon(), Real(0.499));
    const Real g = tanForButterworth(w);
    const Real g2 = g * g;
    for (size_t idx = 0; idx < nSections; ++idx) {
      SectionState& s = states_[idx];
      const Real d = damping[idx];
      const Real y_hp = (input - s.s1 * (g + d) - s.s2) / (Real(1) + d * g + g2);
      const Real y_bp = g * y_hp + s.s1;
      const Real two_g = Real(2) * g;
      s.s1 += two_g * y_hp;
      s.s2 += two_g * y_bp;
      input = y_hp;
    }
    return input;
  }
};

template<typename Real, int maxTap = 256> class DelayAntialiased {
private:
  static_assert(maxTap > 0 && maxTap % 2 == 0);

  static constexpr int minTimeSample = maxTap / 2 - 1;

  std::vector<Real> buf{std::vector<Real>(maxTap, Real(0))};
  Real maxTime = 0;
  Real prevTime = 0;
  int wptr = 0;

public:
  void setup(Real maxTimeSample) {
    maxTime = maxTimeSample;
    buf.resize(std::max(size_t(maxTap), size_t(maxTime) + maxTap / 2 + 1));
  }

  void reset() {
    prevTime = 0;
    wptr = 0;
    std::fill(buf.begin(), buf.end(), Real(0));
  }

  Real process(Real input, Real timeInSample) {
    const int size = int(buf.size());

    // Write to buffer.
    if (++wptr >= size) { wptr = 0; }
    buf[size_t(wptr)] = input;

    // Start reading from buffer. Setup convolution filter parameters.
    const int localTap = std::clamp(2 * int(timeInSample), int(2), maxTap);
    const int halfTap = localTap / 2;
    const Real clamped = std::clamp(timeInSample, Real(halfTap - 1), maxTime);

    const Real timeDiff = std::abs(prevTime - clamped + Real(1));
    prevTime = clamped;
    const Real cutoff = timeDiff <= Real(1) ? Real(0.5) : std::exp2(-timeDiff);

    if (timeInSample <= 0) { return input * Real(2) * cutoff; }

    const int timeInt = int(clamped);
    const Real fraction = clamped - Real(timeInt);
    const Real mid = fraction - halfTap;

    // Setup oscillator 1 (o1). Windowed sinc lowpass.
    constexpr Real pi = std::numbers::pi_v<Real>;
    const Real o1_omega = Real(2) * pi * cutoff;
    const Real o1_k = Real(2) * std::cos(o1_omega);
    Real o1_u1 = std::sin((mid - Real(1)) * o1_omega);
    Real o1_u2 = std::sin((mid - Real(2)) * o1_omega);

    // Setup oscillator 2 (o2). Blackman-Harris window.
    const Real o2_omega = Real(2) * pi / Real(maxTap + 1);
    const Real o2_phi = o2_omega * Real(maxTap / 2 - halfTap);
    const Real o2_k = Real(2) * std::cos(o2_omega);
    Real o2_u1 = std::cos(o2_phi + o2_omega * (Real(-1) + fraction));
    Real o2_u2 = std::cos(o2_phi + o2_omega * (Real(-2) + fraction));

    // Convolution.
    int rptr = wptr - timeInt - halfTap;
    if (rptr < 0) { rptr += size; }

    Real sum = 0;
    const Real theta_scale = Real(2) * cutoff * pi;
    for (int i = 0; i < localTap; ++i) {
      const Real o1_u0 = o1_k * o1_u1 - o1_u2;
      o1_u2 = o1_u1;
      o1_u1 = o1_u0;

      const Real o2_u0 = o2_k * o2_u1 - o2_u2;
      o2_u2 = o2_u1;
      o2_u1 = o2_u0;

      const Real window = Real(0.21747)
        + o2_u0 * (Real(-0.45325) + o2_u0 * (Real(0.28256) + o2_u0 * Real(-0.04672)));

      const Real x = Real(i) + mid;
      const Real theta = theta_scale * x;
      Real sinc;
      if (std::abs(theta) <= Real(0.32)) {
        const Real t2 = theta * theta;

        Real y = Real(-1.0 / 39916800.0);
        y = y * t2 + Real(+1.0 / 362880.0);
        y = y * t2 + Real(-1.0 / 5040.0);
        y = y * t2 + Real(+1.0 / 120.0);
        y = y * t2 + Real(-1.0 / 6.0);
        y = y * t2 + Real(+1.0);

        sinc = Real(2) * cutoff * y;
      } else {
        sinc = o1_u0 / (pi * x);
      }
      sum += sinc * window * buf[size_t(rptr)];
      if (++rptr >= size) { rptr = 0; }
    }
    return sum;
  }
};

template<typename Real> class FeedbackGate {
private:
  Real envelope{};
  Real smoothed{};
  Real riseAlpha{};
  Real fallAlpha{};

public:
  void setup(Real sampleRate) {
    riseAlpha = secondToEmaAlpha(sampleRate, Real(0.001));
    fallAlpha = secondToEmaAlpha(sampleRate, Real(0.02));
  }

  void reset() {
    envelope = Real(0);
    smoothed = Real(0);
  }

  Real process(Real input, Real thresholdOpen, Real thresholdClose) {
    const Real x0 = std::abs(input);

    if (x0 >= thresholdOpen) {
      envelope = Real(1);
    } else if (x0 < thresholdClose) {
      envelope += fallAlpha * (x0 - envelope);
    }

    const bool open = envelope > thresholdClose;
    const Real alpha = open ? riseAlpha : fallAlpha;
    return smoothed += alpha * (envelope - smoothed);
  }
};

#include <cmath>

template<typename Real> class TempoSyncedLfo {
public:
  enum class Synchronization { Phase, Frequency, Free };

private:
  Real frameIndex = Real(-1);
  Real phase = 0;
  Real lfoFreq = 0;
  Real syncAlpha = 0; // Exponential moving average coefficient.
  Real targetFreq = 0;
  Synchronization mode = Synchronization::Phase;

public:
  inline Real getPhase() const { return phase; }
  void setSyncRate(Real emaAlpha) { syncAlpha = emaAlpha; }

  void reset() {
    frameIndex = Real(-1);
    phase = 0;
    lfoFreq = 0;
  }

  void update(Synchronization syncMode, Real tempo, Real lfoRate, Real upper, Real lower,
              Real upRate) {
    mode = syncMode;
    frameIndex = Real(-1);
    const Real activeTempo = (mode == Synchronization::Free) ? Real(120) : tempo;
    targetFreq = (activeTempo * lower * lfoRate) / (Real(60) * upper * upRate);
  }

  Real process(bool isPlaying, bool isResetting, Real initialPhase, Real upRate, Real beatsElapsed,
               Real tempo) {
    frameIndex += Real(1);

    auto output = phase + initialPhase;
    output -= std::floor(output);

    constexpr auto tolerance = Real(1) / Real(1024);

    if (isResetting) [[unlikely]] {
      const auto diff = std::remainder(-phase, Real(1));
      const auto absDiff = std::abs(diff);
      phase += syncAlpha * (absDiff >= tolerance ? absDiff : diff);
    } else if (mode == Synchronization::Phase && !isPlaying) {
      phase += targetFreq;
      phase -= std::floor(phase);
      return output;
    } else {
      phase += lfoFreq;
      phase -= std::floor(phase);

      if (mode == Synchronization::Phase) {
        const auto samplesElapsed = beatsElapsed * (Real(60) * upRate / tempo);
        auto targetPhase = (samplesElapsed + frameIndex) * targetFreq;
        targetPhase -= std::floor(targetPhase);
        const auto diff = std::remainder(targetPhase - phase, Real(1));
        const auto absDiff = std::abs(diff);
        phase += syncAlpha * (absDiff >= tolerance ? absDiff : diff);
      }
    }

    lfoFreq += syncAlpha * (targetFreq - lfoFreq);

    return output;
  }
};

template<typename Real> inline std::complex<Real> boxToCircle(const std::complex<Real>& z) {
  Real ax = std::abs(z.real());
  Real ay = std::abs(z.imag());

  Real scale = (ax > ay) ? ax : ay;
  Real r = std::hypot(ax, ay);

  if (r <= std::numeric_limits<Real>::epsilon()) { return {Real(0), Real(0)}; }
  return z * (scale / r);
}

template<typename Real>
inline Real butterworthNormalizationGain(Real cutoffNormalized, Real maxGain = Real(100)) {
  constexpr Real pi = std::numbers::pi_v<Real>;
  constexpr Real sqrt2 = std::numbers::sqrt2_v<Real>;
  constexpr Real invSqrt2 = Real(1) / sqrt2;

  const Real c = std::clamp(cutoffNormalized, Real(1e-6), Real(0.499));

  const Real w = pi * c;
  const Real g = tanForButterworth(w);
  const Real g2 = g * g;

  const Real num = g2 + sqrt2 * g + Real(1);
  const Real den = g2 + invSqrt2 * g;

  const Real gain = std::sqrt(num / den);

  return std::min(gain, maxGain);
}

template<typename Real> class Fdn2 {
private:
  static constexpr size_t fdnSize = 2;

  std::array<Real, fdnSize> buffer;
  FeedbackGate<Real> feedbackGate;
  std::array<Saturator<Real>, fdnSize> saturator;
  Saturator<Real> inputSaturator;
  std::array<ButterworthHighpass<Real>, fdnSize> safetyHighpass;
  std::array<DelayAntialiased<Real>, fdnSize> delay;
  std::array<ButterworthLowpass<Real, 4>, fdnSize> feedbackLowpass;
  std::array<ButterworthLowpass<Real, 2>, fdnSize> viscosityLowpass;

public:
  void setup(Real maxTimeSamples) {
    for (auto& x : delay) { x.setup(maxTimeSamples); }
  }

  void updateSamplingRate(Real sampleRate) { feedbackGate.setup(sampleRate); }

  void softReset() {
    buffer.fill({});
    feedbackGate.reset();
    for (auto& x : saturator) { x.reset(); }
    inputSaturator.reset();
    for (auto& x : safetyHighpass) { x.reset(); }
    for (auto& x : feedbackLowpass) { x.reset(); }
    for (auto& x : viscosityLowpass) { x.reset(); }
  }

  void reset() {
    softReset();
    for (auto& x : delay) { x.reset(); }
  }

  struct DisplayTime {
    std::array<Real, 2> upper{};
    std::array<Real, 2> lower{};
  };

  struct Parameters {
    Real feedbackGateThreshold;
    Real feedback0;
    Real feedback1;
    Real inputBlend;
    Real timeInSamples0;
    Real timeInSamples1;
    Real viscosityCutoff;
    Real sigModMode;
    Real sigTimeMod0;
    Real sigTimeMod1;
    Real lfoTimeMod0;
    Real lfoTimeMod1;
    Real sigAmpMod0;
    Real sigAmpMod1;
    Real highpassCutoff;
    Real highpassFade;
    Real flangeBlend;
    Real safeFeedback;
    Real flangeSign;
    Real lowpassCutoff;
    Real lowpassFade;
    typename Saturator<Real>::Function saturatorType;
  };

  Real process(Real input, Real lfoPhase, DisplayTime& displayTime, const Parameters& p_) {
    constexpr auto pi = std::numbers::pi_v<Real>;
    constexpr auto twopi = Real(2) * pi;
    const auto omega = twopi * lfoPhase;
    const auto cs = std::lerp(std::cos(omega), Real(1), p_.flangeBlend);
    const auto sn = std::lerp(std::sin(omega), Real(0), p_.flangeBlend);

    const auto timeLfo = std::abs(Real(4) * lfoPhase - Real(2)) - Real(1);

    const auto fbCircular = boxToCircle(std::complex<Real>{p_.feedback0, p_.feedback1});
    auto fb0 = std::lerp(p_.feedback0, fbCircular.real(), p_.safeFeedback);
    auto fb1 = std::lerp(p_.feedback1, fbCircular.imag(), p_.safeFeedback);
    auto sig0 = cs * buffer[0] - sn * buffer[1];
    auto sig1 = sn * buffer[0] + cs * buffer[1];

    const auto fbGate
      = feedbackGate.process(input, p_.feedbackGateThreshold, p_.feedbackGateThreshold * Real(0.5));
    sig0 *= fb0 * fbGate;
    sig1 *= fb1 * fbGate;

    sig0 = saturator[0].process(sig0, p_.saturatorType);
    sig1 = saturator[1].process(sig1, p_.saturatorType);

    const auto inSat = inputSaturator.process(input, p_.saturatorType);
    const auto delayIn0 = p_.inputBlend * inSat + sig0;
    const auto delayIn1 = (Real(1) - p_.inputBlend) * inSat + sig1;
    const auto hp0 = safetyHighpass[0].process(delayIn0, p_.highpassCutoff);
    const auto hp1 = safetyHighpass[1].process(delayIn1, p_.highpassCutoff);

    const auto viscosityGain = butterworthNormalizationGain(p_.viscosityCutoff, Real(1000));
    const auto viscSig0 = viscosityLowpass[0].process(sig0, p_.viscosityCutoff) * viscosityGain;
    const auto viscSig1 = viscosityLowpass[1].process(sig1, p_.viscosityCutoff) * viscosityGain;
    const auto crossModSig0 = std::lerp(inSat, viscSig0, p_.sigModMode);
    const auto crossModSig1 = std::lerp(inSat, viscSig1, p_.sigModMode);

    auto timeMod0
      = p_.timeInSamples0 * std::exp2(p_.lfoTimeMod0 * timeLfo + p_.sigTimeMod0 * crossModSig0);
    auto timeMod1
      = p_.timeInSamples1 * std::exp2(p_.lfoTimeMod1 * timeLfo + p_.sigTimeMod1 * crossModSig1);

    displayTime.upper[0] = std::max(displayTime.upper[0], timeMod0);
    displayTime.upper[1] = std::max(displayTime.upper[1], timeMod1);
    displayTime.lower[0] = std::min(displayTime.lower[0], timeMod0);
    displayTime.lower[1] = std::min(displayTime.lower[1], timeMod1);

    const auto am0 = std::clamp(std::lerp(Real(1), crossModSig0, p_.sigAmpMod0), Real(-2), Real(2));
    const auto am1 = std::clamp(std::lerp(Real(1), crossModSig1, p_.sigAmpMod1), Real(-2), Real(2));
    buffer[0] = delay[0].process(am0 * std::lerp(delayIn0, hp0, p_.highpassFade), timeMod0);
    buffer[1] = delay[1].process(am1 * std::lerp(delayIn1, hp1, p_.highpassFade), timeMod1);

    buffer[0] = feedbackLowpass[0].process(buffer[0], p_.lowpassCutoff);
    buffer[1] = feedbackLowpass[1].process(buffer[1], p_.lowpassCutoff);

    buffer[0] += p_.flangeSign * buffer[1];
    return buffer[0] + (Real(1) - p_.flangeBlend) * buffer[1];
  }
};

} // namespace Uhhyou
