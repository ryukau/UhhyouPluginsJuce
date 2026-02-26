// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/dsp/smoother.hpp"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>
#include <vector>

namespace Uhhyou {

template<typename Sample> struct SplittedBand2 {
  Sample low;
  Sample high;
  SplittedBand2(Sample low_, Sample high_) : low(low_), high(high_) {}
  inline Sample sum() { return low + high; }
};

template<typename Sample, int length = 63> class BandSplitter {
private:
  static_assert(length > 0);

  int wptr = 0;
  std::array<Sample, length> buf{};

public:
  static constexpr int latency = length / 2;

  void reset() {
    wptr = 0;
    buf.fill({});
  }

  SplittedBand2<Sample> process(Sample input, Sample cutoffNormalized) {
    if constexpr (length == 1) { return Sample(2) * cutoffNormalized * input; }

    // Write to buffer.
    if (++wptr >= length) { wptr = 0; }
    buf[wptr] = input;

    // Setup recursive sine oscillator.
    constexpr Sample pi = std::numbers::pi_v<Sample>;
    const Sample omega = Sample(2) * pi * cutoffNormalized;
    const Sample phi = -latency * omega;
    const Sample k = Sample(2) * std::cos(omega);
    Sample u1 = std::sin(phi - omega);
    Sample u2 = std::sin(phi - Sample(2) * omega);

    // Convolution.
    int rptr = wptr;
    Sample sum = 0;
    for (int i = 0; i < length; ++i) {
      const Sample u0 = k * u1 - u2;
      u2 = u1;
      u1 = u0;

      const Sample x = Sample(i) - latency;
      const Sample sinc = x == 0 ? Sample(2) * cutoffNormalized : u0 / (pi * x);

      if (++rptr >= length) { rptr = 0; }
      sum += sinc * buf[rptr];
    }

    int center = wptr - latency;
    if (center < 0) { center += length; }
    return {sum, buf[center] - sum};
  }
};

template<typename Sample> class EnvelopeFollowerExpDecay {
private:
  Sample y = 0;

public:
  void reset() { y = 0; }

  Sample process(Sample input, Sample decayTimeSample, Sample refreshRatio) {
    const Sample time = std::max(Sample(1), decayTimeSample);

    // `(1e-3)^(1/time)` but using `exp` instead of `pow`.
    // The magic number is `log(1e-3) ~= -6.907755278982137`.
    const Sample decay = std::exp(Sample(-6.907755278982137) / time);

    y *= decay;
    const Sample x = std::abs(input);
    if (x >= refreshRatio * y) { y = x; }
    return y;
  }
};

template<typename Sample> class EnvelopeFollowerEmaCascade {
private:
  static constexpr int nCascade = 3;
  std::array<Sample, nCascade> v{};

  inline Sample cutoffToKp(Sample cutoffNormalized) {
    constexpr Sample twopi = Sample(2) * std::numbers::pi_v<Sample>;
    Sample y = Sample(1) - std::cos(twopi * cutoffNormalized);
    return std::sqrt(y * y + Sample(2) * y) - y;
  }

  inline Sample halfRect(Sample x) { return x >= 0 ? x : 0; }

public:
  void reset() { v.fill({}); }

  Sample process(Sample input, Sample cutoffNormalized) {
    input = std::abs(input);
    for (int i = 0; i < nCascade; ++i) {
      v[i] += cutoffToKp(cutoffNormalized * Sample(i + 1)) * (input - v[i]);
    }

    auto d1 = halfRect(v[1] - v[0]);
    auto d2 = halfRect(v[2] - v[1]);

    return std::max(d1, d2);
  }
};

template<typename Sample, int order = 2> class Butterworth {
private:
  static_assert(order > 0 && order % 2 == 0);
  static constexpr int nSection = order / 2;

  std::array<Sample, nSection> x1{};
  std::array<Sample, nSection> x2{};
  std::array<Sample, nSection> y1{};
  std::array<Sample, nSection> y2{};

public:
  void reset() {
    x1.fill({});
    x2.fill({});
    y1.fill({});
    y2.fill({});
  }

  Sample process(Sample x0, Sample cutoffNormalized) {
    constexpr Sample pi = std::numbers::pi_v<Sample>;

    const Sample omega = Sample(2) * pi * std::clamp(cutoffNormalized, Sample(1e-6), Sample(0.499));
    for (int i = 0; i < nSection; ++i) {
      // const Sample Q = Sample(0.5) / std::sin(Sample(2 * idx + 1) * pi /
      // Sample(order));
      const Sample Q = Sample(0.5) / std::cos(pi * Sample(i) / Sample(order));

      const Sample sn = std::sin(omega);
      const Sample cs = std::cos(omega);

      const Sample alpha = sn / (Sample(2) * Q);
      const Sample a0 = Sample(1) + alpha;

      const Sample a1 = (Sample(2) * cs) / a0;
      const Sample a2 = (alpha - Sample(1)) / a0;

      const Sample b1 = (Sample(1) - std::cos(omega)) / a0;
      const Sample b0 = b1 / Sample(2);
      const Sample& b2 = b0;

      const Sample y0 = b0 * x0 + b1 * x1[i] + b2 * x2[i] + a1 * y1[i] + a2 * y2[i];

      x2[i] = x1[i];
      x1[i] = x0;
      y2[i] = y1[i];
      y1[i] = y0;

      x0 = y0;
    }
    return x0;
  }
};

} // namespace Uhhyou
