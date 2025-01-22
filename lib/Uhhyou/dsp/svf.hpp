// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace Uhhyou {

namespace SVFTool {

constexpr double minCutoff = 0.00001;
constexpr double nyquist = 0.49998;
constexpr double maxFlatK
  = double(1) / (std::numbers::sqrt2_v<double> - std::numeric_limits<double>::epsilon());

template<typename T> inline T freqToG(T normalizedFreq)
{
  return T(std::tan(
    std::clamp(double(normalizedFreq), minCutoff, nyquist) * std::numbers::pi_v<double>));
}

template<typename T> inline T qToK(T Q)
{
  if (Q < std::numeric_limits<T>::epsilon()) Q = std::numeric_limits<T>::epsilon();
  return T(1) / Q;
}

} // namespace SVFTool

template<typename Sample> class SVF {
private:
  Sample ic1eq = 0;
  Sample ic2eq = 0;

  inline std::array<Sample, 2> processInternal(Sample v0, Sample g, Sample k)
  {
    Sample v1 = (ic1eq + g * (v0 - ic2eq)) / (Sample(1) + g * (g + k));
    Sample v2 = ic2eq + g * v1;
    ic1eq = Sample(2) * v1 - ic1eq;
    ic2eq = Sample(2) * v2 - ic2eq;
    return {v1, v2};
  }

public:
  void reset()
  {
    ic1eq = 0;
    ic2eq = 0;
  }

  Sample lowpass(Sample v0, Sample g, Sample k)
  {
    auto val = processInternal(v0, g, k);
    return val[1];
  }

  Sample highpass(Sample v0, Sample g, Sample k)
  {
    auto val = processInternal(v0, g, k);
    return v0 - k * val[0] - val[1];
  }
};

} // namespace Uhhyou
