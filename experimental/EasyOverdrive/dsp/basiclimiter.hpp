// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/dsp/smoother.hpp"

#include <algorithm>
#include <random>
#include <vector>

namespace Uhhyou {

// 2 EMA lowpass filters are cascaded.
template<typename Sample> class LimiterReleaseFilter {
public:
  Sample kp = Sample(1);
  Sample v1 = 0;
  Sample v2 = 0;

  void reset(Sample value = 0) {
    v1 = value;
    v2 = value;
  }

  void setMin(Sample value) {
    v1 = std::min(v1, value);
    v2 = std::min(v2, value);
  }

  void setCutoff(Sample sampleRate, Sample cutoffHz) {
    kp = cutoffHz >= sampleRate / Sample(2)
      ? Sample(1)
      : Sample(cutoffToEmaAlpha<double>(double(sampleRate), double(cutoffHz)));
  }

  void setSecond(Sample sampleRate, Sample second) {
    kp = Sample(cutoffToEmaAlpha<double>(double(sampleRate), double(second)));
  }

  Sample getValue() { return v2; }

  Sample process(Sample input) {
    auto&& v0 = input;
    v1 += kp * (v0 - v1);
    v2 += kp * (v1 - v2);
    return v2;
  }

  Sample processKp(Sample input, Sample kp) {
    auto&& v0 = input;
    v1 += kp * (v0 - v1);
    v2 += kp * (v1 - v2);
    return v2;
  }
};

// Integer sample delay.
template<typename Sample> class IntDelay {
private:
  std::vector<Sample> buf_;
  size_t wptr_ = 0;
  size_t rptr_ = 0;

public:
  IntDelay(size_t size = 64) : buf_(size) {}

  void resize(size_t size) {
    buf_.resize(size + 1);
    wptr_ = 0;
    rptr_ = 0;
  }

  void reset() { std::fill(buf_.begin(), buf_.end(), Sample(0)); }

  void setFrames(size_t delayFrames) {
    if (delayFrames >= buf_.size()) { delayFrames = buf_.size(); }
    rptr_ = wptr_ - delayFrames;
    if (rptr_ >= buf_.size()) {
      rptr_ += buf_.size(); // Unsigned negative overflow case.
    }
  }

  Sample process(Sample input) {
    if (++wptr_ >= buf_.size()) { wptr_ = 0; }
    buf_[wptr_] = input;

    if (++rptr_ >= buf_.size()) { rptr_ = 0; }
    return buf_[rptr_];
  }
};

// Replacement of std::deque with reduced memory allocation.
template<typename T> struct RingQueue {
  std::vector<T> buf;
  size_t wptr = 0;
  size_t rptr = 0;

  RingQueue(size_t size = 64) : buf(size) {}

  void resize(size_t size) {
    buf.resize(size);
    wptr = 0;
    rptr = 0;
  }

  void reset(T value = 0) {
    std::fill(buf.begin(), buf.end(), value);
    wptr = 0;
    rptr = 0;
  }

  inline size_t size() {
    auto sz = wptr - rptr;
    if (sz >= buf.size()) {
      sz += buf.size(); // Unsigned overflow case.
    }
    return sz;
  }

  inline bool empty() { return wptr == rptr; }

  T& front() { return buf[increment(rptr)]; }
  T& back() { return buf[wptr]; }

  inline size_t increment(size_t idx) {
    if (++idx >= buf.size()) { idx -= buf.size(); }
    return idx;
  }

  inline size_t decrement(size_t idx) {
    if (--idx >= buf.size()) {
      idx += buf.size(); // Unsigned overflow case.
    }
    return idx;
  }

  void push_back(T value) {
    wptr = increment(wptr);
    buf[wptr] = value;
  }

  T pop_front() {
    rptr = increment(rptr);
    return buf[rptr];
  }

  T pop_back() {
    wptr = decrement(wptr);
    return buf[wptr];
  }
};

/*
Ideal peak hold.
- When `setFrames(0)`, all output becomes 0.
- When `setFrames(1)`, PeakHold will bypass the input.
*/
template<typename Sample> struct PeakHold {
  IntDelay<Sample> delay;
  RingQueue<Sample> queue;

  PeakHold(size_t size = 64) {
    resize(size);
    setFrames(1);
  }

  void resize(size_t size) {
    delay.resize(size);
    queue.resize(size);
  }

  void reset() {
    delay.reset();
    queue.reset();
  }

  void setFrames(size_t frames) { delay.setFrames(frames); }

  Sample process(Sample x0) {
    while (!queue.empty()) {
      if (queue.back() >= x0) { break; }
      queue.pop_back();
    }
    queue.push_back(x0);
    if (delay.process(x0) == queue.front()) { queue.pop_front(); }
    return queue.front();
  }
};

template<typename Sample, bool fastSmoothing = true> class DoubleAverageFilter {
private:
  Sample denom_ = Sample(1);
  Sample sum1_ = 0;
  Sample sum2_ = 0;
  Sample buf_ = 0;
  IntDelay<Sample> delay1_;
  IntDelay<Sample> delay2_;

public:
  void resize(size_t size) {
    delay1_.resize(size / 2 + 1);
    delay2_.resize(size / 2);
  }

  void reset() {
    sum1_ = 0;
    sum2_ = 0;
    buf_ = 0;
    delay1_.reset();
    delay2_.reset();
  }

  void setFrames(size_t frames) {
    auto half = frames / 2;
    denom_ = 1 / Sample((half + 1) * half);
    delay1_.setFrames(half + 1);
    delay2_.setFrames(half);
  }

  inline Sample add(Sample lhs, Sample rhs) {
    if constexpr (fastSmoothing) { return lhs + rhs; }

    if (lhs < rhs) { std::swap(lhs, rhs); }
    int expL;
    std::frexp(lhs, &expL);
    auto cut = std::ldexp(float(1), expL - std::numeric_limits<Sample>::digits);
    auto rounded = rhs - std::fmod(rhs, cut);
    return lhs + rounded;
  }

  Sample process(Sample input) {
    input *= denom_;

    sum1_ = add(sum1_, input);
    Sample d1 = delay1_.process(input);
    sum1_ = std::max(Sample(0), sum1_ - d1);

    sum2_ = add(sum2_, sum1_);
    Sample d2 = delay2_.process(sum1_);
    sum2_ = std::max(Sample(0), sum2_ - d2);

    auto output = buf_;
    buf_ = sum2_;
    return output;
  }
};

template<typename Sample, bool fastSmoothing = true> class BasicLimiter {
private:
  size_t attackFrames_ = 0;
  Sample thresholdAmp_ = Sample(1); // thresholdAmp > 0.

  PeakHold<Sample> peakhold_;
  DoubleAverageFilter<double, fastSmoothing> smoother_;
  LimiterReleaseFilter<Sample> releaseFilter_;
  IntDelay<Sample> lookaheadDelay_;

public:
  size_t latency(size_t upfold) { return attackFrames_ / upfold; }

  void resize(size_t size) {
    size += size % 2; // DoubleAverageFilter requires multiple of 2.
    peakhold_.resize(size);
    smoother_.resize(size);
    lookaheadDelay_.resize(size);
  }

  void reset() {
    peakhold_.reset();
    smoother_.reset();
    releaseFilter_.reset(Sample(1));
    lookaheadDelay_.reset();
  }

  void prepare(Sample sampleRate, Sample attackSeconds, Sample releaseSeconds,
               Sample thresholdAmplitude) {
    auto prevAttack = attackFrames_;
    attackFrames_ = size_t(sampleRate * attackSeconds + Sample(0.5));
    attackFrames_ += attackFrames_ % 2; // DoubleAverageFilter requires multiple of 2.

    if (prevAttack != attackFrames_) { reset(); }

    releaseFilter_.setSecond(sampleRate, releaseSeconds);

    thresholdAmp_ = thresholdAmplitude;

    peakhold_.setFrames(attackFrames_);
    smoother_.setFrames(attackFrames_);
    lookaheadDelay_.setFrames(attackFrames_);
  }

  inline Sample applyCharacteristicCurve(Sample peakAmp) {
    return peakAmp > thresholdAmp_ ? thresholdAmp_ / peakAmp : Sample(1);
  }

  inline Sample processRelease(Sample gain) {
    releaseFilter_.setMin(gain);
    return releaseFilter_.process(gain);
  }

  Sample process(Sample input) {
    auto inAbs = std::fabs(input);
    auto peakAmp = peakhold_.process(inAbs);
    auto candidate = applyCharacteristicCurve(peakAmp);
    auto released = processRelease(candidate);
    auto gainAmp = std::min(released, candidate);
    auto smoothed = smoother_.process(gainAmp);
    auto delayed = lookaheadDelay_.process(input);
    return smoothed * delayed;
  }
};

} // namespace Uhhyou
