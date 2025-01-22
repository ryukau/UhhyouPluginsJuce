// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "./basiclimiter.hpp"
#include "Uhhyou/dsp/smoother.hpp"

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

template<typename T> inline T freqToG(T normalizedFreq) {
  return T(
    std::tan(std::clamp(double(normalizedFreq), minCutoff, nyquist) * std::numbers::pi_v<double>));
}

template<typename T> inline T qToK(T Q) {
  if (Q < std::numeric_limits<T>::epsilon()) { Q = std::numeric_limits<T>::epsilon(); }
  return T(1) / Q;
}

} // namespace SVFTool

template<typename Sample> class SVF {
private:
  Sample ic1eq_ = 0;
  Sample ic2eq_ = 0;

  inline std::array<Sample, 2> processInternal(Sample v0, Sample g, Sample k) {
    Sample v1 = (ic1eq_ + g * (v0 - ic2eq_)) / (Sample(1) + g * (g + k));
    Sample v2 = ic2eq_ + g * v1;
    ic1eq_ = Sample(2) * v1 - ic1eq_;
    ic2eq_ = Sample(2) * v2 - ic2eq_;
    return {v1, v2};
  }

public:
  void reset() {
    ic1eq_ = 0;
    ic2eq_ = 0;
  }

  Sample lowpass(Sample v0, Sample g, Sample k) {
    auto val = processInternal(v0, g, k);
    return val[1];
  }

  Sample highpass(Sample v0, Sample g, Sample k) {
    auto val = processInternal(v0, g, k);
    return v0 - k * val[0] - val[1];
  }
};

// 3rd order Lagrange Interpolation.
// Range of t is in [0, 1]. Interpolates between y1 and y2.
template<typename Sample> inline Sample cubicInterp(const std::array<Sample, 4>& y, Sample t) {
  auto u = 1 + t;
  auto d0 = y[0] - y[1];
  auto d1 = d0 - (y[1] - y[2]);
  auto d2 = d1 - ((y[1] - y[2]) - (y[2] - y[3]));
  return y[0] - ((d2 * (2 - u) / 3 + d1) * (1 - u) / 2 + d0) * u;
}

// BLT: Bilinear transform.
template<typename Sample> class BLTLP1 {
private:
  Sample bn_ = 1;
  Sample a1_ = -1; // Negated.
  Sample x1_ = 0;
  Sample y1_ = 0;

public:
  void reset() {
    x1_ = 0;
    y1_ = 0;
  }

  void setCutoff(Sample sampleRate, Sample cutoffHz) {
    constexpr Sample pi = std::numbers::pi_v<Sample>;
    auto k = std::tan(pi * cutoffHz / sampleRate);
    auto a0 = Sample(1) + Sample(1) / k;
    bn_ = Sample(1) / a0;
    a1_ = (k - Sample(1)) / a0; // Negated.
  }

  Sample process(Sample x0) {
    auto y0 = bn_ * (x0 + x1_) + a1_ * y1_;
    x1_ = x0;
    return y1_ = y0;
  }
};

template<typename T> inline std::array<T, 2> secondToSvfParameters(T sampleRate, T seconds, T Q) {
  auto g = SVFTool::freqToG(T(1) / std::max(sampleRate * seconds, T(4)));
  auto d = T(1) / (T(1) + g * g + g / std::max(Q, std::numeric_limits<T>::epsilon()));
  return {g, d};
}

template<typename Sample> class SVFLP {
private:
  Sample ic1eq_ = 0;
  Sample ic2eq_ = 0;

  ExpSmoother<Sample> svfG_;
  ExpSmoother<Sample> svfD_;
  ExpSmoother<Sample> svfK_;

public:
  SVFLP(const SmootherParameter<Sample>& smoo) : svfG_(smoo), svfD_(smoo), svfK_(smoo) {}

  void reset(Sample sampleRate, Sample seconds, Sample Q) {
    ic1eq_ = 0;
    ic2eq_ = 0;

    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG_.reset(prm[0]);
    svfD_.reset(prm[1]);
    svfK_.reset(Sample(1) / Q);
  }

  void push(Sample sampleRate, Sample seconds, Sample Q) {
    auto prm = secondToSvfParameters(sampleRate, seconds, Q);
    svfG_.push(prm[0]);
    svfD_.push(prm[1]);
    svfK_.push(Sample(1) / Q);
  }

  Sample process(Sample v0) {
    const Sample g = svfG_.process();
    const Sample d = svfD_.process();

    Sample v1 = (ic1eq_ + g * (v0 - ic2eq_)) * d;
    Sample v2 = ic2eq_ + g * v1;
    ic1eq_ = Sample(2) * v1 - ic1eq_;
    ic2eq_ = Sample(2) * v2 - ic2eq_;
    return v2;
  }

  Sample processMod(Sample v0, Sample gMod, Sample resoMod) {
    const Sample g = std::clamp(svfG_.process() * std::exp2(std::min(gMod, Sample(16))), Sample(0),
                                Sample(3000));
    const Sample k = resoMod * svfK_.process();

    Sample v1 = (ic1eq_ + g * (v0 - ic2eq_)) / (Sample(1) + g * g + g * k);
    Sample v2 = ic2eq_ + g * v1;
    ic1eq_ = Sample(2) * v1 - ic1eq_;
    ic2eq_ = Sample(2) * v2 - ic2eq_;
    return v2;

    // return process(v0);
  }
};

template<typename Sample> class Delay {
public:
  size_t wptr = 0;
  std::vector<Sample> buf;

  void resize(size_t maxDelaySample) {
    buf.resize(maxDelaySample < 4 ? 4 : maxDelaySample + 1);
    reset();
  }

  void reset() { std::fill(buf.begin(), buf.end(), Sample(0)); }

  Sample process(Sample input, Sample timeInSample) {
    // Set delay time.
    Sample clamped = std::clamp(timeInSample, Sample(0), Sample(buf.size() - 1));
    size_t&& timeInt = size_t(clamped);
    Sample rFraction = clamped - Sample(timeInt);

    size_t rptr0 = wptr - timeInt;
    size_t rptr1 = rptr0 - 1;
    if (rptr0 >= buf.size()) {
      rptr0 += buf.size(); // Unsigned negative overflow case.
    }
    if (rptr1 >= buf.size()) {
      rptr1 += buf.size(); // Unsigned negative overflow case.
    }

    // Write to buffer.
    buf[wptr] = input;
    if (++wptr >= buf.size()) { wptr -= buf.size(); }

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
  Sample holdValue_ = 0;
  size_t counter_ = 0;
  size_t holdSamples_ = 1024;

  Sample previousPeak_ = 0;
  Sample fractionalDelay_ = 0;
  std::array<Sample, 4> cubicBuffer_{};

  ExpSmoother<Sample> amp_;
  ExpSmoother<Sample> delayTimeSample_;
  SVFLP<Sample> svf_;
  Delay<Sample> delay_;

public:
  BadLimiter(const SmootherParameter<Sample>& smoo)
      : amp_(smoo), delayTimeSample_(smoo), svf_(smoo) {}

  // size_t latency(size_t upfold) { return attackFrames / upfold; }

  void resize(size_t maxDelaySample) { delay_.resize(maxDelaySample); }

  void reset(Sample sampleRate, Sample holdSeconds, Sample Q, Sample characterAmp) {
    holdValue_ = 0;
    counter_ = 0;
    holdSamples_ = size_t(sampleRate * holdSeconds);

    previousPeak_ = 0;
    fractionalDelay_ = 0;
    cubicBuffer_.fill({});

    amp_.reset(characterAmp);
    svf_.reset(sampleRate, holdSeconds, Q);
    delay_.reset();
  }

  void push(Sample sampleRate, Sample holdSeconds, Sample Q, Sample characterAmp) {
    amp_.push(characterAmp);
    svf_.push(sampleRate, holdSeconds, Q);
    delayTimeSample_.push(sampleRate * holdSeconds);
  }

  inline Sample poly(Sample x0, Sample ratio) {
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

  inline Sample forwardHold(Sample absed, Sample spike) {
    ++counter_;
    if (counter_ > holdSamples_ || holdValue_ < absed) {
      holdValue_ = absed - spike;
      counter_ = 0;
    }
    return counter_ ? holdValue_ : holdValue_ + spike;
  }

  inline Sample forwardHoldSpike(Sample absed, Sample spike) {
    ++counter_;
    if (counter_ > holdSamples_ || holdValue_ < absed) {
      holdValue_ = absed;
      counter_ = 0;
    }
    return counter_ ? holdValue_ : holdValue_ + spike;
  }

  inline Sample forwardHoldSpikeCubic(Sample absed, Sample spike) {
    ++counter_;
    if (counter_ > holdSamples_ || holdValue_ < absed) {
      holdValue_ = absed;
      counter_ = 0;

      fractionalDelay_ = absed > std::numeric_limits<Sample>::epsilon()
        ? std::clamp((holdValue_ - previousPeak_) / absed, Sample(0), Sample(1))
        : Sample(1);
    }
    previousPeak_ = absed;

    std::rotate(cubicBuffer_.rbegin(), cubicBuffer_.rbegin() + 1, cubicBuffer_.rend());
    cubicBuffer_[0] = counter_ ? 0 : spike;

    return holdValue_ + cubicInterp(cubicBuffer_, fractionalDelay_);
  }

  inline Sample forwardHoldHardGate(Sample absed, Sample spike) {
    ++counter_;
    if (counter_ > holdSamples_ || holdValue_ < absed) {
      holdValue_ = absed - Sample(1000) * spike;
      counter_ = 0;
    }
    return holdValue_;
  }

  inline Sample getGainSigmoid(Sample peak) {
    if (peak < std::numeric_limits<Sample>::epsilon()) { return 0; }
    return Sample(1) + std::erf(peak) * (Sample(1) / peak - Sample(1));
  }

  inline Sample getGainHardClip(Sample peak) {
    const auto th = amp_.process();
    if (peak > th) { return th / peak; }
    return Sample(1);
  }

  Sample processImmediate(Sample x0) {
    Sample peak = forwardHold(std::abs(x0), amp_.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processHardGate(Sample x0) {
    Sample peak = forwardHoldHardGate(std::abs(x0), amp_.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processSpike(Sample x0) {
    Sample peak = forwardHoldSpike(std::abs(x0), amp_.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processSpikeCubic(Sample x0) {
    Sample peak = forwardHoldSpikeCubic(std::abs(x0), amp_.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processCutoffMod(Sample x0) {
    Sample peak = forwardHoldSpike(std::abs(x0), Sample(0));
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.processMod(gain, x0 * amp_.process(), Sample(1)));
    Sample output = smoothed * x0;
    return output;
  }

  Sample processMatched(Sample x0) {
    Sample peak = forwardHold(std::abs(x0), amp_.process());
    Sample gain = getGainSigmoid(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample delayed = delay_.process(x0, delayTimeSample_.process());
    Sample output = smoothed * (delayed + std::erf(x0));
    return output;
  }

  Sample processBadLimiter(Sample x0) {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample gain = getGainHardClip(peak);
    Sample smoothed = std::abs(svf_.process(gain));
    Sample delayed = delay_.process(x0, delayTimeSample_.process());
    Sample output = smoothed * (delayed + std::erf(x0));
    return output;
  }

  Sample processPolyDrive(Sample x0) {
    Sample peak = forwardHold(std::abs(x0), Sample(0));
    Sample smoothed = std::abs(svf_.process(peak));
    Sample delayed = delay_.process(x0, delayTimeSample_.process());
    Sample ratio = std::clamp(smoothed - Sample(1), Sample(0), Sample(1));
    Sample output = poly(amp_.process() * (delayed + std::erf(x0)), ratio);
    return output;
  }
};

template<typename Sample> class AsymmetricDrive {
private:
  static constexpr Sample eps_ = std::numeric_limits<Sample>::epsilon();

  Sample x1_ = 0;
  Sample accP_ = 0; // Positive accumulator.
  Sample accN_ = 0; // Negative accumulator.
  ExpSmoother<Sample> decayP_;
  ExpSmoother<Sample> decayN_;
  ExpSmoother<Sample> exponentRange_;

  SVFLP<Sample> svf_;

public:
  AsymmetricDrive(const SmootherParameter<Sample>& smoo)
      : decayP_(smoo), decayN_(smoo), exponentRange_(smoo), svf_(smoo) {}

  void reset(Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q, Sample expRange) {
    x1_ = 0;
    accP_ = 0;
    accN_ = 0;

    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP_.reset(std::pow(eps_, inv_frames));
    decayN_.reset(std::pow(eps_, inv_frames * decayBias));

    exponentRange_.reset(expRange);

    svf_.reset(sampleRate, decaySecond, Q);
  }

  void push(Sample sampleRate, Sample decaySecond, Sample decayBias, Sample Q, Sample expRange) {
    Sample inv_frames = Sample(1) / std::max(sampleRate * decaySecond, Sample(2));
    decayP_.push(std::pow(eps_, inv_frames));
    decayN_.push(std::pow(eps_, inv_frames * decayBias));

    exponentRange_.push(expRange);

    svf_.push(sampleRate, decaySecond, Q);
  }

  Sample process(Sample x0) {
    Sample d0 = x0 - x1_;
    x1_ = x0;

    decayP_.process();
    decayN_.process();

    if (d0 > 0) {
      accP_ += d0;
      accN_ *= decayN_.value();
    } else {
      accN_ += d0;
      accP_ *= decayP_.value();
    }

    Sample r = exponentRange_.process();
    Sample t = std::clamp(svf_.process(accP_ + accN_) * r, Sample(-r), Sample(r));
    return x0 * std::exp2(t);
  }
};

} // namespace Uhhyou
