// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../common/dsp/basiclimiter.hpp"
#include "../../common/dsp/smoother.hpp"
#include "../../common/dsp/svf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace Uhhyou {

template<typename T>
inline std::array<T, 2> secondToSvfParameters(T sampleRate, T seconds, T Q)
{
  auto g = SVFTool::freqToG(T(1) / std::max(sampleRate * seconds, T(4)));
  auto d = T(1) / (T(1) + g * g + g / std::max(Q, std::numeric_limits<T>::epsilon()));
  return {g, d};
}

template<typename Sample> class SVFLP {
private:
  Sample ic1eq = 0;
  Sample ic2eq = 0;

  ExpSmoother<Sample> svfG;
  ExpSmoother<Sample> svfD;

public:
  void reset(Sample sampleRate, Sample seconds, Sample Q)
  {
    ic1eq = 0;
    ic2eq = 0;

    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG.reset(prm[0]);
    svfD.reset(prm[1]);
  }

  void push(Sample sampleRate, Sample seconds, Sample Q)
  {
    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG.push(prm[0]);
    svfD.push(prm[1]);
  }

  Sample process(Sample v0)
  {
    const Sample g = svfG.process();
    const Sample d = svfD.process();

    Sample v1 = (ic1eq + g * (v0 - ic2eq)) * d;
    Sample v2 = ic2eq + g * v1;
    ic1eq = Sample(2) * v1 - ic1eq;
    ic2eq = Sample(2) * v2 - ic2eq;
    return v2;
  }
};

template<typename Sample> class Delay {
public:
  size_t wptr = 0;
  std::vector<Sample> buf;

  void resize(size_t maxDelaySample)
  {
    buf.resize(maxDelaySample < 4 ? 4 : maxDelaySample + 1);
    reset();
  }

  void reset() { std::fill(buf.begin(), buf.end(), Sample(0)); }

  Sample process(Sample input, Sample timeInSample)
  {
    // Set delay time.
    Sample clamped = std::clamp(timeInSample, Sample(0), Sample(buf.size() - 1));
    size_t &&timeInt = size_t(clamped);
    Sample rFraction = clamped - Sample(timeInt);

    size_t rptr0 = wptr - timeInt;
    size_t rptr1 = rptr0 - 1;
    if (rptr0 >= buf.size()) rptr0 += buf.size(); // Unsigned negative overflow case.
    if (rptr1 >= buf.size()) rptr1 += buf.size(); // Unsigned negative overflow case.

    // Write to buffer.
    buf[wptr] = input;
    if (++wptr >= buf.size()) wptr -= buf.size();

    // Read from buffer.
    return buf[rptr0] + rFraction * (buf[rptr1] - buf[rptr0]);
  }
};

template<typename Sample> class BadLimiter {
private:
  Sample holdValue = 0;
  size_t counter = 0;
  size_t holdSamples = 1024;

  ExpSmoother<Sample> amp;
  ExpSmoother<Sample> delayTimeSample;
  SVFLP<Sample> svf;
  Delay<Sample> delay;

public:
  // size_t latency(size_t upfold) { return attackFrames / upfold; }

  void resize(size_t maxDelaySample) { delay.resize(maxDelaySample); }

  void reset(Sample sampleRate, Sample holdSeconds, Sample Q, Sample characterAmp)
  {
    holdValue = 0;
    counter = 0;
    holdSamples = size_t(sampleRate * holdSeconds);

    amp.reset(characterAmp);
    svf.reset(sampleRate, holdSeconds, Q);
    delay.reset();
  }

  void push(Sample sampleRate, Sample holdSeconds, Sample Q, Sample characterAmp)
  {
    amp.push(characterAmp);
    svf.push(sampleRate, holdSeconds, Q);
    delayTimeSample.push(sampleRate * holdSeconds);
  }

  inline Sample poly(Sample x0, Sample ratio)
  {
    constexpr Sample a1 = Sample(0.761468168789663);
    constexpr Sample a2 = Sample(0.4497752742394532);
    constexpr Sample a3 = Sample(-0.520876400831698);
    constexpr Sample a4 = Sample(0.11531086075727837);

    Sample p1 = a1 + ratio * (Sample(1) - a1);
    Sample p2 = a2 - ratio * a2;
    Sample p3 = a3 - ratio * a3;
    Sample p4 = a4 - ratio * a4;

    Sample x = std::abs(x0);
    return std::copysign(x * (p1 + x * (p2 + x * (p3 + x * p4))), x0);
  }

  inline Sample forwardHold(Sample absed, Sample spike)
  {
    ++counter;
    if (counter > holdSamples || holdValue < absed) {
      holdValue = absed - spike;
      counter = 0;
    }
    return holdValue;
  }

  inline Sample getGainSigmoid(Sample peak)
  {
    if (peak < std::numeric_limits<Sample>::epsilon()) return 0;
    return Sample(1) + std::erf(peak) * (Sample(1) / peak - Sample(1));
  }

  inline Sample getGainHardClip(Sample peak)
  {
    const auto th = amp.process();
    if (peak > th) return th / peak;
    return Sample(1);
  }

  Sample processImmediate(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    return smoothed * x0;
  }

  Sample processMatched(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    return smoothed * (delayed + std::erf(x0));
  }

  Sample processBadLimiter(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample gain = getGainHardClip(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    return smoothed * (delayed + std::erf(x0));
  }

  Sample processPolyDrive(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample smoothed = std::abs(svf.process(peak));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    Sample ratio = std::clamp(smoothed - Sample(1), Sample(0), Sample(1));
    return poly(amp.process() * (delayed + std::erf(x0)), ratio);
  }
};

template<typename Sample> class AsymmetryDrive {
private:
  static constexpr Sample eps = std::numeric_limits<Sample>::epsilon();

  Sample x1 = 0;
  Sample accP = 0; // Positive accumulator.
  Sample accN = 0; // Negative accumulator.
  ExpSmoother<Sample> decayP;
  ExpSmoother<Sample> decayN;

  SVFLP<Sample> svf;

public:
  void reset(Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q)
  {
    x1 = 0;
    accP = 0;
    accN = 0;

    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP.reset(std::pow(eps, inv_frames));
    decayN.reset(std::pow(eps, inv_frames * decayBias));

    svf.reset(sampleRate, decaySecond, Q);
  }

  void push(Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q)
  {
    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP.push(std::pow(eps, inv_frames));
    decayN.push(std::pow(eps, inv_frames * decayBias));

    svf.push(sampleRate, decaySecond, Q);
  }

  Sample process(Sample x0)
  {
    Sample d0 = x0 - x1;
    x1 = x0;

    decayP.process();
    decayN.process();

    if (d0 > 0) {
      accP += d0;
      accN *= decayN.v();
    } else {
      accN += d0;
      accP *= decayP.v();
    }

    Sample t = accP + accN;
    t = svf.process(t);
    t = std::clamp(t, Sample(0), Sample(1));

    Sample s0 = x0 * x0;
    Sample s1 = std::sqrt(std::abs(x0));
    return std::copysign(s0 + t * (s1 - s0), x0);
  }
};

} // namespace Uhhyou
