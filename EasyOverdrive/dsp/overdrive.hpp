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

// 3rd order Lagrange Interpolation.
// Range of t is in [0, 1]. Interpolates between y1 and y2.
template<typename Sample>
inline Sample cubicInterp(const std::array<Sample, 4> &y, Sample t)
{
  auto u = 1 + t;
  auto d0 = y[0] - y[1];
  auto d1 = d0 - (y[1] - y[2]);
  auto d2 = d1 - ((y[1] - y[2]) - (y[2] - y[3]));
  return y[0] - ((d2 * (2 - u) / 3 + d1) * (1 - u) / 2 + d0) * u;
}

// BLT: Bilinear transform.
template<typename Sample> class BLTLP1 {
private:
  Sample bn = 1;
  Sample a1 = -1; // Negated.
  Sample x1 = 0;
  Sample y1 = 0;

public:
  void reset()
  {
    x1 = 0;
    y1 = 0;
  }

  void setCutoff(Sample sampleRate, Sample cutoffHz)
  {
    constexpr Sample pi = std::numbers::pi_v<Sample>;
    auto k = std::tan(pi * cutoffHz / sampleRate);
    auto a0 = Sample(1) + Sample(1) / k;
    bn = Sample(1) / a0;
    a1 = (k - Sample(1)) / a0; // Negated.
  }

  Sample process(Sample x0)
  {
    y0 = bn * (x0 + x1) + a1 * y1;
    x1 = x0;
    return y1 = y0;
  }
};

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
  ExpSmoother<Sample> svfK;

public:
  void reset(Sample sampleRate, Sample seconds, Sample Q)
  {
    ic1eq = 0;
    ic2eq = 0;

    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG.reset(prm[0]);
    svfD.reset(prm[1]);
    svfK.reset(Sample(1) / Q);
  }

  void push(Sample sampleRate, Sample seconds, Sample Q)
  {
    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG.push(prm[0]);
    svfD.push(prm[1]);
    svfK.push(Sample(1) / Q);
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

  Sample processMod(Sample v0, Sample gMod, Sample resoMod)
  {
    const Sample g = std::clamp(
      svfG.process() * std::exp2(std::min(gMod, Sample(16))), Sample(0), Sample(3000));
    const Sample k = resoMod * svfK.process();

    Sample v1 = (ic1eq + g * (v0 - ic2eq)) / (Sample(1) + g * g + g * k);
    Sample v2 = ic2eq + g * v1;
    ic1eq = Sample(2) * v1 - ic1eq;
    ic2eq = Sample(2) * v2 - ic2eq;
    return v2;

    // return process(v0);
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

namespace BadLimiterType {
enum BadLimiterType : size_t {
  Immediate,
  HardGate,
  Spike,
  SpikeCubic,
  CutoffMod,
  Matched,
  BadLimiter,
  PolyDrive,
};
} // namespace BadLimiterType

template<typename Sample> class BadLimiter {
private:
  Sample holdValue = 0;
  size_t counter = 0;
  size_t holdSamples = 1024;

  Sample previousPeak = 0;
  Sample fractionalDelay = 0;
  std::array<Sample, 4> cubicBuffer{};

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

    previousPeak = 0;
    fractionalDelay = 0;
    cubicBuffer.fill({});

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
    return counter ? holdValue : holdValue + spike;
  }

  inline Sample forwardHoldSpike(Sample absed, Sample spike)
  {
    ++counter;
    if (counter > holdSamples || holdValue < absed) {
      holdValue = absed;
      counter = 0;
    }
    return counter ? holdValue : holdValue + spike;
  }

  inline Sample forwardHoldSpikeCubic(Sample absed, Sample spike)
  {
    ++counter;
    if (counter > holdSamples || holdValue < absed) {
      holdValue = absed;
      counter = 0;

      fractionalDelay = absed > std::numeric_limits<Sample>::epsilon()
        ? std::clamp((holdValue - previousPeak) / absed, Sample(0), Sample(1))
        : Sample(1);
    }
    previousPeak = absed;

    std::rotate(cubicBuffer.rbegin(), cubicBuffer.rbegin() + 1, cubicBuffer.rend());
    cubicBuffer[0] = counter ? 0 : spike;

    return holdValue + cubicInterp(cubicBuffer, fractionalDelay);
  }

  inline Sample forwardHoldHardGate(Sample absed, Sample spike)
  {
    ++counter;
    if (counter > holdSamples || holdValue < absed) {
      holdValue = absed - Sample(1000) * spike;
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
    Sample output = smoothed * x0;
    return output;
  }

  Sample processHardGate(Sample x0)
  {
    Sample peak = forwardHoldHardGate(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processSpike(Sample x0)
  {
    Sample peak = forwardHoldSpike(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processSpikeCubic(Sample x0)
  {
    Sample peak = forwardHoldSpikeCubic(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processCutoffMod(Sample x0)
  {
    Sample peak = forwardHoldSpike(std::abs(x0), Sample(0));
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.processMod(gain, x0 * amp.process(), Sample(1)));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processMatched(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), amp.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    Sample output = smoothed * (delayed + std::erf(x0));
    return output;
  }

  Sample processBadLimiter(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample gain = getGainHardClip(peak);
    Sample smoothed = std::abs(svf.process(gain));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    Sample output = smoothed * (delayed + std::erf(x0));
    return output;
  }

  Sample processPolyDrive(Sample x0)
  {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample smoothed = std::abs(svf.process(peak));
    Sample delayed = delay.process(x0, delayTimeSample.process());
    Sample ratio = std::clamp(smoothed - Sample(1), Sample(0), Sample(1));
    Sample output = poly(amp.process() * (delayed + std::erf(x0)), ratio);
    return output;
  }
};

template<typename Sample> class AsymmetricDrive {
private:
  static constexpr Sample eps = std::numeric_limits<Sample>::epsilon();

  Sample x1 = 0;
  Sample accP = 0; // Positive accumulator.
  Sample accN = 0; // Negative accumulator.
  ExpSmoother<Sample> decayP;
  ExpSmoother<Sample> decayN;
  ExpSmoother<Sample> exponentRange;

  SVFLP<Sample> svf;

public:
  void reset(
    Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q, Sample expRange)
  {
    x1 = 0;
    accP = 0;
    accN = 0;

    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP.reset(std::pow(eps, inv_frames));
    decayN.reset(std::pow(eps, inv_frames * decayBias));

    exponentRange.reset(expRange);

    svf.reset(sampleRate, decaySecond, Q);
  }

  void
  push(Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q, Sample expRange)
  {
    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP.push(std::pow(eps, inv_frames));
    decayN.push(std::pow(eps, inv_frames * decayBias));

    exponentRange.push(expRange);

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

    Sample r = exponentRange.process();
    Sample t = std::clamp(svf.process(accP + accN) * r, Sample(-r), Sample(r));
    return x0 * std::exp2(t);
  }
};

} // namespace Uhhyou
