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

  std::vector<Real> buf_{std::vector<Real>(maxTap, Real(0))};
  Real maxTime_ = 0;
  Real prevTime_ = 0;
  int wptr_ = 0;

public:
  void setup(Real maxTimeSample) {
    maxTime_ = maxTimeSample;
    buf_.resize(std::max(size_t(maxTap), size_t(maxTime_) + maxTap / 2 + 1));
  }

  void reset() {
    prevTime_ = 0;
    wptr_ = 0;
    std::fill(buf_.begin(), buf_.end(), Real(0));
  }

  Real process(Real input, Real timeInSample) {
    const int size = int(buf_.size());

    // Write to buffer.
    if (++wptr_ >= size) { wptr_ = 0; }
    buf_[size_t(wptr_)] = input;

    // Start reading from buffer. Setup convolution filter parameters.
    const int localTap = std::clamp(2 * int(timeInSample), int(2), maxTap);
    const int halfTap = localTap / 2;
    const Real clamped = std::clamp(timeInSample, Real(halfTap - 1), maxTime_);

    const Real timeDiff = std::abs(prevTime_ - clamped + Real(1));
    prevTime_ = clamped;
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
    int rptr = wptr_ - timeInt - halfTap;
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
      sum += sinc * window * buf_[size_t(rptr)];
      if (++rptr >= size) { rptr = 0; }
    }
    return sum;
  }
};

template<typename Real> class FeedbackGate {
private:
  Real envelope_{};
  Real smoothed_{};
  Real riseAlpha_{};
  Real fallAlpha_{};

public:
  void setup(Real sampleRate) {
    riseAlpha_ = secondToEmaAlpha(sampleRate, Real(0.001));
    fallAlpha_ = secondToEmaAlpha(sampleRate, Real(0.02));
  }

  void reset() {
    envelope_ = Real(0);
    smoothed_ = Real(0);
  }

  Real process(Real input, Real thresholdOpen, Real thresholdClose) {
    const Real x0 = std::abs(input);

    if (x0 >= thresholdOpen) {
      envelope_ = Real(1);
    } else if (x0 < thresholdClose) {
      envelope_ += fallAlpha_ * (x0 - envelope_);
    }

    const bool open = envelope_ > thresholdClose;
    const Real alpha = open ? riseAlpha_ : fallAlpha_;
    return smoothed_ += alpha * (envelope_ - smoothed_);
  }
};

#include <cmath>

template<typename Real> class TempoSyncedLfo {
public:
  enum class Synchronization { Phase, Frequency, Free };

private:
  Real frameIndex_ = Real(-1);
  Real phase_ = 0;
  Real lfoFreq_ = 0;
  Real syncAlpha_ = 0; // Exponential moving average coefficient.
  Real targetFreq_ = 0;
  Synchronization mode_ = Synchronization::Phase;

public:
  inline Real getPhase() const { return phase_; }
  void setSyncRate(Real emaAlpha) { syncAlpha_ = emaAlpha; }

  void reset() {
    frameIndex_ = Real(-1);
    phase_ = 0;
    lfoFreq_ = 0;
  }

  void update(Synchronization syncMode, Real tempo, Real lfoRate, Real upper, Real lower,
              Real upRate) {
    mode_ = syncMode;
    frameIndex_ = Real(-1);
    const Real activeTempo = (mode_ == Synchronization::Free) ? Real(120) : tempo;
    targetFreq_ = (activeTempo * lower * lfoRate) / (Real(60) * upper * upRate);
  }

  Real process(bool isPlaying, bool isResetting, Real initialPhase, Real upRate, Real beatsElapsed,
               Real tempo) {
    frameIndex_ += Real(1);

    auto output = phase_ + initialPhase;
    output -= std::floor(output);

    constexpr auto tolerance = Real(1) / Real(1024);

    if (isResetting) [[unlikely]] {
      const auto diff = std::remainder(-phase_, Real(1));
      const auto absDiff = std::abs(diff);
      phase_ += syncAlpha_ * (absDiff >= tolerance ? absDiff : diff);
    } else if (mode_ == Synchronization::Phase && !isPlaying) {
      phase_ += targetFreq_;
      phase_ -= std::floor(phase_);
      return output;
    } else {
      phase_ += lfoFreq_;
      phase_ -= std::floor(phase_);

      if (mode_ == Synchronization::Phase) {
        const auto samplesElapsed = beatsElapsed * (Real(60) * upRate / tempo);
        auto targetPhase = (samplesElapsed + frameIndex_) * targetFreq_;
        targetPhase -= std::floor(targetPhase);
        const auto diff = std::remainder(targetPhase - phase_, Real(1));
        const auto absDiff = std::abs(diff);
        phase_ += syncAlpha_ * (absDiff >= tolerance ? absDiff : diff);
      }
    }

    lfoFreq_ += syncAlpha_ * (targetFreq_ - lfoFreq_);

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

  std::array<Real, fdnSize> buffer_;
  FeedbackGate<Real> feedbackGate_;
  std::array<Saturator<Real>, fdnSize> saturator_;
  Saturator<Real> inputSaturator_;
  std::array<ButterworthHighpass<Real>, fdnSize> safetyHighpass_;
  std::array<DelayAntialiased<Real>, fdnSize> delay_;
  std::array<ButterworthLowpass<Real, 4>, fdnSize> feedbackLowpass_;
  std::array<ButterworthLowpass<Real, 2>, fdnSize> viscosityLowpass_;

public:
  void setup(Real maxTimeSamples) {
    for (auto& x : delay_) { x.setup(maxTimeSamples); }
  }

  void updateSamplingRate(Real sampleRate) { feedbackGate_.setup(sampleRate); }

  void softReset() {
    buffer_.fill({});
    feedbackGate_.reset();
    for (auto& x : saturator_) { x.reset(); }
    inputSaturator_.reset();
    for (auto& x : safetyHighpass_) { x.reset(); }
    for (auto& x : feedbackLowpass_) { x.reset(); }
    for (auto& x : viscosityLowpass_) { x.reset(); }
  }

  void reset() {
    softReset();
    for (auto& x : delay_) { x.reset(); }
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

  Real process(Real input, Real lfoPhase, DisplayTime& displayTime, const Parameters& params) {
    constexpr auto pi = std::numbers::pi_v<Real>;
    constexpr auto twopi = Real(2) * pi;
    const auto omega = twopi * lfoPhase;
    const auto cs = std::lerp(std::cos(omega), Real(1), params.flangeBlend);
    const auto sn = std::lerp(std::sin(omega), Real(0), params.flangeBlend);

    const auto timeLfo = std::abs(Real(4) * lfoPhase - Real(2)) - Real(1);

    const auto fbCircular = boxToCircle(std::complex<Real>{params.feedback0, params.feedback1});
    auto fb0 = std::lerp(params.feedback0, fbCircular.real(), params.safeFeedback);
    auto fb1 = std::lerp(params.feedback1, fbCircular.imag(), params.safeFeedback);
    auto sig0 = cs * buffer_[0] - sn * buffer_[1];
    auto sig1 = sn * buffer_[0] + cs * buffer_[1];

    const auto fbGate = feedbackGate_.process(input, params.feedbackGateThreshold,
                                              params.feedbackGateThreshold * Real(0.5));
    sig0 *= fb0 * fbGate;
    sig1 *= fb1 * fbGate;

    sig0 = saturator_[0].process(sig0, params.saturatorType);
    sig1 = saturator_[1].process(sig1, params.saturatorType);

    const auto inSat = inputSaturator_.process(input, params.saturatorType);
    const auto delayIn0 = params.inputBlend * inSat + sig0;
    const auto delayIn1 = (Real(1) - params.inputBlend) * inSat + sig1;
    const auto hp0 = safetyHighpass_[0].process(delayIn0, params.highpassCutoff);
    const auto hp1 = safetyHighpass_[1].process(delayIn1, params.highpassCutoff);

    const auto viscosityGain = butterworthNormalizationGain(params.viscosityCutoff, Real(1000));
    const auto viscSig0
      = viscosityLowpass_[0].process(sig0, params.viscosityCutoff) * viscosityGain;
    const auto viscSig1
      = viscosityLowpass_[1].process(sig1, params.viscosityCutoff) * viscosityGain;
    const auto crossModSig0 = std::lerp(inSat, viscSig0, params.sigModMode);
    const auto crossModSig1 = std::lerp(inSat, viscSig1, params.sigModMode);

    auto timeMod0 = params.timeInSamples0
      * std::exp2(params.lfoTimeMod0 * timeLfo + params.sigTimeMod0 * crossModSig0);
    auto timeMod1 = params.timeInSamples1
      * std::exp2(params.lfoTimeMod1 * timeLfo + params.sigTimeMod1 * crossModSig1);

    displayTime.upper[0] = std::max(displayTime.upper[0], timeMod0);
    displayTime.upper[1] = std::max(displayTime.upper[1], timeMod1);
    displayTime.lower[0] = std::min(displayTime.lower[0], timeMod0);
    displayTime.lower[1] = std::min(displayTime.lower[1], timeMod1);

    const auto am0
      = std::clamp(std::lerp(Real(1), crossModSig0, params.sigAmpMod0), Real(-2), Real(2));
    const auto am1
      = std::clamp(std::lerp(Real(1), crossModSig1, params.sigAmpMod1), Real(-2), Real(2));
    buffer_[0] = delay_[0].process(am0 * std::lerp(delayIn0, hp0, params.highpassFade), timeMod0);
    buffer_[1] = delay_[1].process(am1 * std::lerp(delayIn1, hp1, params.highpassFade), timeMod1);

    buffer_[0] = feedbackLowpass_[0].process(buffer_[0], params.lowpassCutoff);
    buffer_[1] = feedbackLowpass_[1].process(buffer_[1], params.lowpassCutoff);

    buffer_[0] += params.flangeSign * buffer_[1];
    return buffer_[0] + (Real(1) - params.flangeBlend) * buffer_[1];
  }
};

} // namespace Uhhyou
