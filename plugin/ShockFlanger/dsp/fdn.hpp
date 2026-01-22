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
template<typename T> inline T tanForButterworth(T x)
{
  const T x2 = x * x;
  return -(
    (x * (((T(36) * x2 - T(6930)) * x2 + T(270270)) * x2 - T(2027025))
     / ((((x2 - T(630)) * x2 + T(51975)) * x2 - T(945945)) * x2 + T(2027025))));
}

template<typename Real, int order = 2> class ButterworthLowpass {
  static_assert(order > 0 && order % 2 == 0);

private:
  static constexpr int nSections = order / 2;

  static inline const std::array<Real, nSections> damping = []()
  {
    std::array<Real, nSections> d{};
    for (int i = 0; i < nSections; ++i) {
      const Real theta
        = (std::numbers::pi_v<Real> * (Real(2) * i + Real(1))) / (Real(2) * order);
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

  Real process(Real input, Real cutoffNormalized)
  {
    const Real c
      = std::clamp(cutoffNormalized, std::numeric_limits<Real>::epsilon(), Real(0.499));
    const Real w = std::numbers::pi_v<Real> * c;
    const Real g = tanForButterworth(w);
    const Real g2 = g * g;
    for (size_t idx = 0; idx < nSections; ++idx) {
      SectionState &s = states_[idx];
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
  static constexpr int nSections = order / 2;

  static inline const std::array<Real, nSections> damping = []()
  {
    std::array<Real, nSections> d{};
    for (int i = 0; i < nSections; ++i) {
      const Real theta
        = (std::numbers::pi_v<Real> * (Real(2) * i + Real(1))) / (Real(2) * order);
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

  Real process(Real input, Real cutoffNormalized)
  {
    const Real w = std::numbers::pi_v<Real>
      * std::clamp(cutoffNormalized, std::numeric_limits<Real>::epsilon(), Real(0.499));
    const Real g = tanForButterworth(w);
    const Real g2 = g * g;
    for (int idx = 0; idx < nSections; ++idx) {
      SectionState &s = states_[idx];
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

template<typename Real> class AdaptiveNotchCPZ {
private:
  Real alpha = Real(-2);
  Real v1 = 0;
  Real v2 = 0;

public:
  void reset()
  {
    alpha = Real(-2); // 0 Hz as initial guess.
    v1 = 0;
    v2 = 0;
  }

  // `width = pi * bandwidth_hz / fs`.
  // `tracking = K / fs`. It should be that `tracking ~< 2 / (input * input)`.
  Real process(Real input, Real width, Real tracking)
  {
    const auto a1 = width * alpha;
    const auto a2 = width * width;
    auto gain = (Real(1) + std::abs(a1) + a2) / (Real(2) + std::abs(alpha));

    constexpr auto clip = Real(1) / std::numeric_limits<Real>::epsilon();
    const auto x0 = std::clamp(input, -clip, clip);

    auto v0 = x0 - a1 * v1 - a2 * v2;
    const auto y0 = v0 + alpha * v1 + v2;
    const auto s0 = (Real(1) - width) * v0 - width * (Real(1) - width) * v2;
    constexpr auto bound = Real(2);
    alpha = std::clamp(alpha - y0 * s0 * tracking, -bound, bound);

    v2 = v1;
    v1 = v0;

    return y0 * gain;
  }
};

template<typename T> inline T lagrange3Interp(T y0, T y1, T y2, T y3, T t)
{
  auto u = T(1) + t;
  auto d0 = y0 - y1;
  auto d1 = d0 - (y1 - y2);
  auto d2 = d1 - ((y1 - y2) - (y2 - y3));
  return y0 - u * (d0 + (T(1) - u) / T(2) * (d1 + (T(2) - u) / T(3) * d2));
}

template<typename Real> class Delay {
private:
  int wptr = 0;
  std::vector<Real> buf{Real(0), Real(0)};

public:
  void setup(Real maxTimeSamples)
  {
    buf.resize(std::max(size_t(4), size_t(maxTimeSamples) + 4));
    reset();
  }

  void reset() { std::fill(buf.begin(), buf.end(), Real(0)); }

  Real process(Real input, Real timeInSamples)
  {
    const int size = int(buf.size());
    const Real clamped = std::clamp(timeInSamples - Real(1), Real(1), Real(size - 4));
    const int timeInt = int(clamped);
    const Real rFraction = clamped - Real(timeInt);

    // Write to buffer.
    if (++wptr >= size) wptr = 0;
    buf[wptr] = input;

    // Read from buffer.
    auto rptr0 = wptr - timeInt;
    auto rptr1 = rptr0 - 1;
    auto rptr2 = rptr0 - 2;
    auto rptr3 = rptr0 - 3;
    if (rptr0 < 0) rptr0 += size;
    if (rptr1 < 0) rptr1 += size;
    if (rptr2 < 0) rptr2 += size;
    if (rptr3 < 0) rptr3 += size;
    return lagrange3Interp(buf[rptr0], buf[rptr1], buf[rptr2], buf[rptr3], rFraction);
  }
};

template<typename Real, int maxTap = 256> class DelayAntialiased {
private:
  static_assert(maxTap > 0 && maxTap % 2 == 0);

  static constexpr int minTimeSample = maxTap / 2 - 1;

  Real maxTime = 0;
  Real prevTime = 0;
  int wptr = 0;
  std::vector<Real> buf{std::vector<Real>(maxTap, Real(0))};

public:
  void setup(Real maxTimeSample)
  {
    maxTime = maxTimeSample;
    buf.resize(std::max(size_t(maxTap), size_t(maxTime) + maxTap / 2 + 1));
  }

  void reset()
  {
    prevTime = 0;
    wptr = 0;
    std::fill(buf.begin(), buf.end(), Real(0));
  }

  Real process(Real input, Real timeInSample)
  {
    const int size = int(buf.size());

    // Write to buffer.
    if (++wptr >= size) wptr = 0;
    buf[wptr] = input;

    // Start reading from buffer. Setup convolution filter parameters.
    const int localTap = std::clamp(2 * int(timeInSample), int(2), maxTap);
    const int halfTap = localTap / 2;
    const Real clamped = std::clamp(timeInSample, Real(halfTap - 1), maxTime);

    const Real timeDiff = std::abs(prevTime - clamped + Real(1));
    prevTime = clamped;
    const Real cutoff = timeDiff <= Real(1) ? Real(0.5) : std::exp2(-timeDiff);

    if (timeInSample <= 0) return input * Real(2) * cutoff;

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
    if (rptr < 0) rptr += size;

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
      sum += sinc * window * buf[rptr];
      if (++rptr >= size) rptr = 0;
    }
    return sum;
  }

  Real processReference(Real input, Real timeInSample)
  {
    constexpr Real pi = std::numbers::pi_v<Real>;

    const auto modifiedSinc = [&](Real x, Real cutoff)
    {
      const Real u = Real(2) * cutoff * x;
      const Real theta = pi * u;
      if (std::abs(theta) < Real(0.32)) {
        const Real t2 = theta * theta;

        Real y = Real(-1) / Real(39916800);
        y = y * t2 + Real(+1) / Real(362880);
        y = y * t2 + Real(-1) / Real(5040);
        y = y * t2 + Real(+1) / Real(120);
        y = y * t2 + Real(-1) / Real(6);
        y = y * t2 + Real(+1);

        return Real(2) * cutoff * y;
      }

      const auto k = std::llrint(u);
      return Real(1 - 2 * (k & 1)) * std::sin(pi * (u - Real(k))) / (pi * x);
    };

    const int size = int(buf.size());

    // Write to buffer.
    if (++wptr >= size) wptr = 0;
    buf[wptr] = input;

    // Read from buffer.
    const int localTap = std::clamp(2 * int(timeInSample), int(2), maxTap);
    const int halfTap = localTap / 2;
    const Real clamped = std::clamp(timeInSample, Real(halfTap - 1), maxTime);

    const Real timeDiff = std::abs(prevTime - clamped + Real(1));
    prevTime = clamped;
    Real cutoff = timeDiff <= Real(1) ? Real(0.5) : std::exp2(-timeDiff);
    if (timeInSample <= 0) return input * Real(2) * cutoff;

    const int timeInt = int(clamped);
    const Real fraction = clamped - Real(timeInt);
    const Real mid = fraction - halfTap;

    // Convolution.
    int rptr = wptr - timeInt - halfTap;
    if (rptr < 0) rptr += size;

    Real sum = 0;
    const Real window_omega = Real(2) * pi / Real(maxTap + 1);
    const Real window_phase = window_omega * Real(maxTap / 2 - halfTap);
    for (int i = 0; i < localTap; ++i) {
      const Real cn = std::cos((Real(i) + fraction) * window_omega + window_phase);
      const Real window = Real(0.21747)
        + cn * (Real(-0.45325) + cn * (Real(0.28256) + cn * Real(-0.04672)));

      const Real sinc = modifiedSinc(Real(i) + mid, cutoff);
      sum += sinc * window * buf[rptr];
      if (++rptr >= size) rptr = 0;
    }
    return sum;
  }

  Real processVersinc(Real input, Real timeInSample)
  {
    const int size = int(buf.size());

    // Write to buffer.
    if (++wptr >= size) wptr = 0;
    buf[wptr] = input;

    // Start reading from buffer. Setup convolution filter parameters.
    const int localTap = std::clamp(2 * int(timeInSample), int(2), maxTap);
    const int halfTap = localTap / 2;
    const Real clamped = std::clamp(timeInSample, Real(halfTap - 1), maxTime);

    const Real timeDiff = std::abs(prevTime - clamped + Real(1));
    prevTime = clamped;
    const Real cutoff = timeDiff <= Real(1) ? Real(0.5) : std::exp2(-timeDiff);

    if (timeInSample <= 0) return input * Real(2) * cutoff;

    const int timeInt = int(clamped);
    const Real fraction = clamped - Real(timeInt);
    const Real mid = fraction - halfTap;

    // Setup oscillator 1 (o1). Windowed versinc lowpass.
    constexpr Real pi = std::numbers::pi_v<Real>;
    const Real o1_omega = pi * cutoff;
    const Real o1_phi = mid * o1_omega;
    const Real o1_k = Real(2) * std::cos(o1_omega);
    Real o1_u1 = std::sin(o1_phi - o1_omega);
    Real o1_u2 = std::sin(o1_phi - Real(2) * o1_omega);

    // Setup oscillator 2 (o2). Blackman-Harris window.
    const Real o2_omega = Real(2) * pi / Real(maxTap + 1);
    const Real o2_phi = o2_omega * Real(maxTap / 2 - halfTap);
    const Real o2_k = Real(2) * std::cos(o2_omega);
    Real o2_u1 = std::cos(o2_phi + o2_omega * (Real(-1) + fraction));
    Real o2_u2 = std::cos(o2_phi + o2_omega * (Real(-2) + fraction));

    // Convolution.
    int rptr = wptr - timeInt - halfTap;
    if (rptr < 0) rptr += size;

    Real sum = 0;
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
      const Real theta = pi * cutoff * x;
      Real versinc;
      if (std::abs(theta) <= Real(0.1) * pi) {
        const Real t2 = theta * theta;
        Real poly = Real(-2.0 / 467775.0);
        poly = poly * t2 + Real(+2.0 / 14175.0);
        poly = poly * t2 + Real(-1.0 / 315.0);
        poly = poly * t2 + Real(+2.0 / 45.0);
        poly = poly * t2 + Real(-1.0 / 3.0);
        poly = poly * t2 + Real(+1.0);
        versinc = (Real(2) * cutoff * theta) * poly;
      } else {
        versinc = Real(2) * o1_u0 * o1_u0 / (pi * x);
      }
      sum += versinc * window * buf[rptr];
      if (++rptr >= size) rptr = 0;
    }
    return sum;
  }

  Real readCubic(Real timeInSamples)
  {
    const int size = int(buf.size());
    const Real clamped = std::clamp(timeInSamples - Real(1), Real(1), Real(size - 4));
    const int timeInt = int(clamped);
    const Real rFraction = clamped - Real(timeInt);

    auto rptr0 = wptr - timeInt;
    auto rptr1 = rptr0 - 1;
    auto rptr2 = rptr0 - 2;
    auto rptr3 = rptr0 - 3;
    if (rptr0 < 0) rptr0 += size;
    if (rptr1 < 0) rptr1 += size;
    if (rptr2 < 0) rptr2 += size;
    if (rptr3 < 0) rptr3 += size;
    return lagrange3Interp(buf[rptr0], buf[rptr1], buf[rptr2], buf[rptr3], rFraction);
  }
};

template<typename Real> class FeedbackGate {
private:
  Real envelope{};
  Real smoothed{};
  Real riseAlpha{};
  Real fallAlpha{};

public:
  void setup(Real sampleRate)
  {
    riseAlpha = secondToEmaAlpha(sampleRate, Real(0.001));
    fallAlpha = secondToEmaAlpha(sampleRate, Real(0.02));
  }

  void reset()
  {
    envelope = Real(0);
    smoothed = Real(0);
  }

  Real process(Real input, Real thresholdOpen, Real thresholdClose)
  {
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

template<typename Real> class LfoPhase {
private:
  Real phase = 0;
  Real lfoFreq = 0;

public:
  void reset(Real initialPhase, Real targetFreq)
  {
    phase = initialPhase;
    lfoFreq = targetFreq;
  }

  // `syncRate` is coefficient of exponential moving average (EMA).
  Real processSync(Real targetPhase, Real targetFreq, Real syncRate)
  {
    phase += lfoFreq;
    phase -= std::floor(phase);

    // Sync phase.
    constexpr auto tolerance = Real(1) / Real(1024);
    const auto d1 = targetPhase - phase;
    const auto diff = std::remainder(d1, Real(1));
    const auto absed = std::abs(diff);
    phase += syncRate * (absed >= tolerance ? absed : diff);

    // Sync freq.
    lfoFreq += syncRate * (targetFreq - lfoFreq);

    return phase;
  }

  Real process(Real freqNormalized)
  {
    const auto output = phase;
    phase += freqNormalized;
    phase -= std::floor(phase);
    return output;
  }
};

template<typename Real> class TempoSyncedLfo {
private:
  LfoPhase<Real> lfo;
  Real syncAlpha = 0;
  Real targetFreq = 0;
  Real frameIndex = Real(-1);

public:
  void setSyncRate(Real emaAlpha) { syncAlpha = emaAlpha; }

  void reset(Real initialPhase)
  {
    frameIndex = Real(-1);
    lfo.reset(initialPhase, targetFreq);
  }

  // Must be called at the start of each processing cycle.
  void update(Real tempo, Real lfoRate, Real upper, Real lower, Real upRate)
  {
    frameIndex = Real(-1);

    // `targetFreq` is a simplified form of the following expressions.
    //
    // ```
    // syncBeat       = upper / (lower * lfoRate);
    // secondsPerBeat = 60 / tempoInBpm;
    // targetFreq     = 1 / (syncBeat * secondPerBeat * sampleRate);
    // ```
    targetFreq = (tempo * lower * lfoRate) / (Real(60) * upper * upRate);
  }

  Real
  process(bool isPlaying, Real upRate, Real beatsElapsed, Real tempo, Real initialPhase)
  {
    frameIndex += Real(1);

    if (!isPlaying) return lfo.process(targetFreq);

    auto samplesElapsed = upRate * beatsElapsed * 60 / tempo;
    auto targetPhase = (samplesElapsed + frameIndex) * targetFreq;
    targetPhase -= std::floor(targetPhase);
    return lfo.processSync(targetPhase + initialPhase, targetFreq, syncAlpha);
  }
};

template<typename T> inline std::complex<T> boxToCircle(const std::complex<T> &z)
{
  T ax = std::abs(z.real());
  T ay = std::abs(z.imag());

  T scale = (ax > ay) ? ax : ay;
  T r = std::hypot(ax, ay);

  if (r <= std::numeric_limits<T>::epsilon()) return {T(0), T(0)};
  return z * (scale / r);
}

template<typename Real> class Fdn2 {
public:
  static constexpr size_t fdnSize = 2;

  std::array<Real, fdnSize> buffer;
  FeedbackGate<Real> feedbackGate;
  std::array<Saturator<Real>, fdnSize> saturator;
  std::array<AdaptiveNotchCPZ<Real>, fdnSize> notch;
  Saturator<Real> inputSaturator;
  std::array<ButterworthHighpass<Real>, fdnSize> safetyHighpass;
  std::array<DelayAntialiased<Real>, fdnSize> delay;
  std::array<ButterworthLowpass<Real, 4>, fdnSize> feedbackLowpass;

  void setup(Real maxTimeSamples)
  {
    for (auto &x : delay) x.setup(maxTimeSamples);
  }

  void updateSamplingRate(Real sampleRate) { feedbackGate.setup(sampleRate); }

  void softReset()
  {
    buffer.fill({});
    feedbackGate.reset();
    for (auto &x : saturator) x.reset();
    for (auto &x : notch) x.reset();
    inputSaturator.reset();
    for (auto &x : safetyHighpass) x.reset();
    for (auto &x : feedbackLowpass) x.reset();
  }

  void reset()
  {
    softReset();
    for (auto &x : delay) x.reset();
  }

  struct Parameters {
    Real feedbackGateThreshold; // Calculated value, not the smoother
    Real feedback0;             // [-1, 1].
    Real feedback1;             // [-1, 1].
    Real notchMix;              // [0, 1].
    Real notchWidth;            // (0, 1).
    Real notchTracking;
    typename Saturator<Real>::Function saturatorType;
    Real timeInSamples0;
    Real timeInSamples1;
    Real timeModOctave0;
    Real timeModOctave1;
    Real crossModOctave0;
    Real crossModOctave1;
    Real highpassCutoff;
    Real highpassFade;
    Real flangeMode;
    Real safeFeedback;
    Real flangeSign;
    Real lowpassCutoff;
    Real lowpassFade;
  };

  Real process(Real input, Real lfoPhase, const Parameters &p_)
  {
    constexpr auto pi = std::numbers::pi_v<Real>;
    constexpr auto twopi = Real(2) * pi;
    const auto omega = twopi * lfoPhase;
    const auto cs = std::lerp(std::cos(omega), Real(1), p_.flangeMode);
    const auto sn = std::lerp(std::sin(omega), Real(0), p_.flangeMode);

    auto triPhi = lfoPhase + Real(0.75);
    triPhi -= std::floor(triPhi);
    const auto timeLfo = std::abs(Real(4) * triPhi - Real(2)) - Real(1);

    const auto fbCircular = boxToCircle(std::complex<Real>{p_.feedback0, p_.feedback1});
    auto fb0 = std::lerp(p_.feedback0, fbCircular.real(), p_.safeFeedback);
    auto fb1 = std::lerp(p_.feedback1, fbCircular.imag(), p_.safeFeedback);
    auto sig0 = cs * buffer[0] - sn * buffer[1];
    auto sig1 = sn * buffer[0] + cs * buffer[1];

    const auto fbGate = feedbackGate.process(
      input, p_.feedbackGateThreshold, p_.feedbackGateThreshold * Real(0.5));
    sig0 *= fb0 * fbGate;
    sig1 *= fb1 * fbGate;

    sig0 = saturator[0].process(sig0, p_.saturatorType);
    sig1 = saturator[1].process(sig1, p_.saturatorType);

    sig0 = std::lerp(
      sig0, notch[0].process(sig0, p_.notchWidth, p_.notchTracking), p_.notchMix);
    sig1 = std::lerp(
      sig1, notch[1].process(sig1, p_.notchWidth, p_.notchTracking), p_.notchMix);

    constexpr auto g1 = Real(0); // [0, 1].
    constexpr auto g2 = Real(1) - g1;
    input = inputSaturator.process(input, p_.saturatorType);

    const auto delayIn0 = g1 * input + sig0;
    const auto delayIn1 = g2 * input + sig1;
    const auto hp0 = safetyHighpass[0].process(delayIn0, p_.highpassCutoff);
    const auto hp1 = safetyHighpass[1].process(delayIn1, p_.highpassCutoff);
    const auto timeMod0 = p_.timeInSamples0
      * std::exp2(p_.timeModOctave0 * timeLfo + p_.crossModOctave0 * input);
    const auto timeMod1 = p_.timeInSamples1
      * std::exp2(p_.timeModOctave1 * timeLfo + p_.crossModOctave1 * input);
    buffer[0] = delay[0].process(std::lerp(delayIn0, hp0, p_.highpassFade), timeMod0);
    buffer[1] = delay[1].process(std::lerp(delayIn1, hp1, p_.highpassFade), timeMod1);

    buffer[0] = feedbackLowpass[0].process(buffer[0], p_.lowpassCutoff);
    buffer[1] = feedbackLowpass[1].process(buffer[1], p_.lowpassCutoff);

    buffer[0] += p_.flangeSign * buffer[1];
    return buffer[0] + (Real(1) - p_.flangeMode) * buffer[1];
  }
};

} // namespace Uhhyou
