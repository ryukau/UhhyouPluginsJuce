// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace Uhhyou {

/**
1-pole matched high-shelving filter.

Reference:
- https://vicanek.de/articles/ShelvingFits.pdf
  - Martin Vicanek, "Matched One-Pole Digital Shelving Filters", revised 2019-09-24.
*/
template<typename Sample> class MatchedHighShelf1 {
private:
  static constexpr Sample kp = Sample(0.0013081403895582485);
  std::array<Sample, 3> coTarget{}; // Target coefficients {b0, b1, -a1}.
  std::array<Sample, 3> coV{};      // Current coefficients.

  Sample x1 = 0;
  Sample y1 = 0;

public:
  Sample nyquistGain() const
  {
    return (coTarget[0] - coTarget[1]) / (Sample(1) + coTarget[2]);
  }

  void push(Sample cutoffNormalized, Sample gainAmp)
  {
    using S = Sample;

    constexpr S minCut = S(10.0 / 48000.0);
    constexpr S maxCut = S(20000.0 / 44100.0);
    if (cutoffNormalized < minCut) {
      cutoffNormalized = minCut;
      gainAmp = S(1);
    } else if (cutoffNormalized > maxCut) {
      cutoffNormalized = maxCut;
      gainAmp = S(1);
    }

    constexpr S pi = std::numbers::pi_v<S>;
    constexpr S phim = S(1.9510565162951536); // 1 - cos(pi * 0.9).
    constexpr S pp = S(2) / (pi * pi);
    constexpr S xi = pp / (phim * phim) - S(1) / phim;

    const S fc2 = cutoffNormalized * cutoffNormalized / S(4);
    S alpha = xi + pp / (gainAmp * fc2);
    S beta = xi + pp * gainAmp / fc2;

    S neg_a1 = alpha / (S(1) + alpha + std::sqrt(S(1) + S(2) * alpha));
    S b = -beta / (S(1) + beta + std::sqrt(S(1) + S(2) * beta));
    coTarget[0] = (S(1) - neg_a1) / (S(1) + b); // b0
    coTarget[1] = b * coTarget[0];              // b1
    coTarget[2] = neg_a1;                       // -a1
  }

  void reset(Sample cutoffNormalized, Sample gainAmp)
  {
    push(cutoffNormalized, gainAmp);
    coV = coTarget;

    x1 = 0;
    y1 = 0;
  }

  Sample process(Sample x0)
  {
    for (size_t i = 0; i < coV.size(); ++i) coV[i] += kp * (coTarget[i] - coV[i]);

    auto y0 = coV[0] * x0 + coV[1] * x1 + coV[2] * y1;
    x1 = x0;
    y1 = y0;
    return y0;
  }
};

template<typename Sample, size_t nCascade> class SlopeFilter {
private:
  std::array<MatchedHighShelf1<Sample>, nCascade> filters;

  static constexpr Sample kp = Sample(0.0013081403895582485);
  Sample gainTarget = Sample(1);
  Sample gainV = Sample(1);

  inline Sample lowshelfGain()
  {
    Sample gain = Sample(1);
    for (const auto &flt : filters) gain *= flt.nyquistGain();
    return Sample(1) / std::max(gain, std::numeric_limits<Sample>::epsilon());
  }

public:
#define SET_SLOPE_FILTER_PARAMETERS(METHOD)                                              \
  if (isHighshelf) {                                                                     \
    Sample gainAmp = std::pow(Sample(10), slopeDecibel / Sample(20));                    \
    Sample cutoff = startHz / sampleRate;                                                \
    for (auto &flt : filters) {                                                          \
      flt.push(cutoff, gainAmp);                                                         \
      cutoff *= Sample(2);                                                               \
    }                                                                                    \
    gainTarget = outputGain;                                                             \
  } else { /* Low shelf */                                                               \
    Sample gainAmp = std::pow(Sample(10), -slopeDecibel / Sample(20));                   \
    Sample cutoff = startHz / sampleRate;                                                \
    for (auto &flt : filters) {                                                          \
      flt.push(cutoff, gainAmp);                                                         \
      cutoff *= Sample(0.5);                                                             \
    }                                                                                    \
    gainTarget = outputGain * lowshelfGain();                                            \
  }

  void push(
    Sample sampleRate,
    Sample startHz,
    Sample slopeDecibel,
    Sample outputGain,
    bool isHighshelf)
  {
    SET_SLOPE_FILTER_PARAMETERS(push);
  }

  void reset(
    Sample sampleRate,
    Sample startHz,
    Sample slopeDecibel,
    Sample outputGain,
    bool isHighshelf)
  {
    SET_SLOPE_FILTER_PARAMETERS(reset);
    gainV = gainTarget;
  }

#undef SET_SLOPE_FILTER_PARAMETERS

  Sample process(Sample x0)
  {
    for (auto &flt : filters) x0 = flt.process(x0);

    gainV += kp * (gainTarget - gainV);
    return gainV * x0;
  }
};

} // namespace Uhhyou
