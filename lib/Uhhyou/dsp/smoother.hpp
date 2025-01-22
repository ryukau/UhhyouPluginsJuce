// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace Uhhyou {

// cutoffNormalized = (cutoff in Hz) / (sampling rate). The value range is [0.0, 0.5).
template<typename T> inline T cutoffToEmaAlpha(T cutoffNormalized) {
  const auto sn = std::sin(std::numbers::pi_v<T> * cutoffNormalized);
  return T(2) * sn / (std::sqrt(sn * sn + T(1)) + sn);
}

template<typename T> inline T cutoffToEmaAlpha(T sampleRate, T cutoffHz) {
  return cutoffToEmaAlpha(cutoffHz / sampleRate);
}

template<typename T> inline T secondToEmaAlpha(T sampleRate, T second) {
  if (second < std::numeric_limits<T>::epsilon()) { return T(1); }
  return cutoffToEmaAlpha(T(1) / (second * sampleRate));
}

// Exponential moving average filter.
template<typename Sample> class EMAFilter {
public:
  Sample kp = Sample(1); // In [0, 1].
  Sample value = 0;

  void setCutoff(Sample sampleRate, Sample cutoffHz) {
    kp = Sample(cutoffToEmaAlpha<double>(double(sampleRate), double(cutoffHz)));
  }

  void setP(Sample p) { kp = std::clamp<Sample>(p, Sample(0), Sample(1)); }
  void reset(Sample v = 0) { value = v; }
  Sample process(Sample input) { return value += kp * (input - value); }
  Sample processKp(Sample input, Sample k) { return value += k * (input - value); }
};

template<typename Sample> class SmootherParameter {
private:
  Sample timeInSamples_{0};
  Sample kp_{0};

public:
  void setTime(Sample sampleRate, Sample seconds) {
    timeInSamples_ = seconds * sampleRate;
    kp_ = secondToEmaAlpha(sampleRate, seconds);
  }

  inline Sample time() const { return timeInSamples_; }
  inline Sample kp() const { return kp_; }
};

template<typename Sample> class ExpSmoother {
private:
  const SmootherParameter<Sample>& param_;
  Sample value_ = 0;
  Sample target_ = 0;

public:
  explicit ExpSmoother(const SmootherParameter<Sample>& param) : param_(param) {}

  inline Sample value() { return value_; }
  inline Sample target() { return target_; }

  void reset(Sample value = 0) {
    value_ = value;
    target_ = value;
  }

  void push(Sample target) { target_ = target; }
  Sample process() { return value_ += param_.kp() * (target_ - value_); }
};

template<typename Sample> class ExpSmootherLocal {
private:
  Sample value_{0};
  Sample target_{0};

public:
  inline Sample value() { return value_; }

  void reset(Sample value = 0) {
    value_ = value;
    target_ = value;
  }

  void push(Sample target) { target_ = target; }
  Sample process(Sample kp) { return value_ += kp * (target_ - value_); }
};

template<typename Sample, size_t length> class ParallelExpSmoother {
private:
  const SmootherParameter<Sample>& param_;
  alignas(32) std::array<Sample, length> value_{};
  alignas(32) std::array<Sample, length> target_{};

public:
  explicit ParallelExpSmoother(const SmootherParameter<Sample>& param) : param_(param) {}

  inline Sample operator[](size_t index) const { return value_[index]; }

  void reset(Sample value = 0) {
    value_.fill(value);
    target_.fill(value);
  }

  void resetAt(size_t index, Sample value = 0) {
    value_[index] = value;
    target_[index] = value;
  }

  void push(Sample target) { target_.fill(target); }
  void pushAt(size_t index, Sample target) { target_[index] = target; }

  void process() {
    for (size_t i = 0; i < length; ++i) { value_[i] += param_.kp() * (target_[i] - value_[i]); }
  }
};

template<typename Sample> class RotaryExpSmoother {
private:
  const SmootherParameter<Sample>& param_;
  Sample value_{0};
  Sample target_{0};

public:
  explicit RotaryExpSmoother(const SmootherParameter<Sample>& param) : param_(param) {}

  inline Sample value() const { return value_; }

  void reset(Sample value = 0) {
    value_ = value;
    target_ = value;
  }

  void push(Sample target) { target_ = target; }

  Sample process() {
    value_ += param_.kp() * std::remainder(target_ - value_, Sample(1));
    value_ -= std::floor(value_);
    return value_;
  }
};

template<typename Sample> class RateLimiter {
private:
  Sample target_ = 0;
  Sample value_ = 0;

public:
  inline Sample value() { return value_; }

  void reset(Sample value = 0) {
    value_ = value;
    target_ = value;
  }

  void push(Sample target) { target_ = target; }

  Sample process(Sample rate) {
    const Sample absRate = std::abs(rate);
    return value_ += std::clamp(target_ - value_, -absRate, absRate);
  }

  Sample process(Sample value, Sample rate) {
    push(value);
    return process(rate);
  }
};

template<typename Sample, size_t length> class ParallelRateLimiter {
private:
  alignas(32) std::array<Sample, length> target_{};
  alignas(32) std::array<Sample, length> value_{};

public:
  inline Sample operator[](size_t index) const { return value_[index]; }

  void resetAt(size_t index, Sample value = 0) {
    value_[index] = value;
    target_[index] = value;
  }

  void pushAt(size_t index, Sample value) { target_[index] = value; }

  void process(Sample rate) {
    const Sample absRate = std::abs(rate);
    for (size_t i = 0; i < length; ++i) {
      const Sample diff = target_[i] - value_[i];
      value_[i] += std::clamp(diff, -absRate, absRate);
    }
  }
};

} // namespace Uhhyou
