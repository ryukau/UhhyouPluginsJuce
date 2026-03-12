// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <numbers>

namespace Uhhyou {

template<typename Sample, int length> class FixedIntDelay {
private:
  int ptr_ = 0;
  std::array<Sample, length> buf_{};

public:
  void reset(Sample value = 0) {
    ptr_ = 0;
    buf_.fill(value);
  }

  Sample process(Sample x0) {
    if (++ptr_ >= length) { ptr_ = 0; }
    Sample output = buf_[ptr_];
    buf_[ptr_] = x0;
    return output;
  }
};

template<typename Sample, size_t fullStage, size_t recStage> struct ComplexIIRDelay;

template<typename Sample, size_t fullStage> struct ComplexIIRDelay<Sample, fullStage, 0> {
  void reset() {}

  using C = std::complex<Sample>;
  inline C process1PoleForward(C x0, const std::array<C, fullStage>&) { return x0; }
  inline C process1PoleReversed(C x0, const std::array<C, fullStage>&) { return x0; }
};

template<typename Sample, size_t fullStage, size_t recStage> struct ComplexIIRDelay {
  static constexpr size_t index = fullStage - recStage;
  FixedIntDelay<std::complex<Sample>, size_t(1) << index> delay;
  ComplexIIRDelay<Sample, fullStage, recStage - 1> recursion;

  void reset() {
    delay.reset();
    recursion.reset();
  }

  inline std::complex<Sample>
  process1PoleForward(std::complex<Sample> x0,
                      const std::array<std::complex<Sample>, fullStage>& poles) {
    return recursion.process1PoleForward(x0 + poles[index] * delay.process(x0), poles);
  }

  inline std::complex<Sample>
  process1PoleReversed(std::complex<Sample> x0,
                       const std::array<std::complex<Sample>, fullStage>& poles) {
    return recursion.process1PoleReversed(poles[index] * x0 + delay.process(x0), poles);
  }
};

template<typename Sample, size_t stage = 8> class ComplexIIR {
private:
  static constexpr size_t nPoles = stage - 1;

  Sample a_per_b_ = 0;
  std::array<std::complex<Sample>, stage> poles_;

  std::complex<Sample> x1_ = 0;
  ComplexIIRDelay<Sample, stage, stage - 1> delay_;

public:
  void reset() {
    x1_ = 0;
    delay_.reset();
  }

  void prepare(std::complex<Sample> pole) {
    a_per_b_ = pole.real() / pole.imag();
    for (auto& value : poles_) {
      value = pole;
      pole *= pole;
    }
  }

  std::complex<Sample> process1PoleForward(Sample x0) {
    std::complex<Sample> sig = x0 + poles_[0] * x1_;
    x1_ = x0;
    return delay_.process1PoleForward(sig, poles_);
  }

  std::complex<Sample> process1PoleReversed(Sample x0) {
    std::complex<Sample> sig = poles_[0] * x0 + x1_;
    x1_ = x0;
    return delay_.process1PoleReversed(sig, poles_);
  }

  Sample process2PoleForward(Sample x0) {
    std::complex<Sample> sig = process1PoleForward(x0);
    return sig.real() + a_per_b_ * sig.imag();
  }

  Sample process2PoleReversed(Sample x0) {
    std::complex<Sample> sig = process1PoleReversed(x0);
    return sig.real() + a_per_b_ * sig.imag();
  }
};

template<typename Sample, size_t order, size_t stage = 8> class LinkwitzRileyFIR {
private:
  static constexpr size_t nSection = order / 4;

  std::array<ComplexIIR<Sample, stage>, nSection> reverse_;
  std::array<ComplexIIR<Sample, stage>, nSection> forward_;

  std::array<Sample, nSection> u1_{};
  std::array<Sample, nSection> u2_{};
  std::array<Sample, nSection> v1_{};
  std::array<Sample, nSection> v2_{};

  Sample gain_ = Sample(1);

public:
  static constexpr size_t latency = nSection * (size_t(1) << stage) + 1;

  LinkwitzRileyFIR() { static_assert(order % 4 == 0 && order >= 4); }

  void reset() {
    for (auto& x : reverse_) { x.reset(); }
    for (auto& x : forward_) { x.reset(); }

    u1_.fill({});
    u2_.fill({});
    v1_.fill({});
    v2_.fill({});
  }

  void prepare(Sample normalizedCrossover) {
    constexpr Sample pi = std::numbers::pi_v<Sample>;
    constexpr size_t N = 2 * nSection; // Butterworth order.

    gain_ = Sample(1);

    auto cutoffRadian = Sample(2) * pi * normalizedCrossover;
    for (size_t idx = 0; idx < nSection; ++idx) {
      auto m = Sample(2 * idx) - Sample(N) + Sample(1);
      auto analogPole = cutoffRadian * std::polar(Sample(-1), pi * m / Sample(2 * N));
      auto pole = (Sample(2) + analogPole) / (Sample(2) - analogPole);
      reverse_[idx].prepare(pole);
      forward_[idx].prepare(pole);
      gain_ *= (Sample(1) + Sample(-2) * pole.real() + std::norm(pole)) / Sample(4);
    }

    gain_ = std::pow(gain_, Sample(1) / Sample(nSection));
  }

  Sample process(Sample x0) {
    constexpr Sample a1 = Sample(2); // -2 for highpass.

    for (size_t i = 0; i < nSection; ++i) {
      Sample u0 = reverse_[i].process2PoleReversed(x0 * gain_);
      x0 = u0 + a1 * u1_[i] + u2_[i];
      u2_[i] = u1_[i];
      u1_[i] = u0;

      Sample v0 = forward_[i].process2PoleForward(x0 * gain_);
      x0 = v0 + a1 * v1_[i] + v2_[i];
      v2_[i] = v1_[i];
      v1_[i] = v0;
    }
    return x0;
  }
};

template<typename Sample, size_t order, size_t stage = 8> class LinkwitzRileyFIR2Band4n {
public:
  static constexpr size_t latency = LinkwitzRileyFIR<Sample, order, stage>::latency;

private:
  LinkwitzRileyFIR<Sample, order, stage> lowpass_;
  FixedIntDelay<Sample, latency> highpassDelay_;

public:
  std::array<Sample, 2> output{}; // 0: low, 1: high.

  LinkwitzRileyFIR2Band4n() { static_assert(order % 4 == 0 && order >= 4); }

  size_t getLatency() { return latency; }

  void reset() {
    lowpass_.reset();
    highpassDelay_.reset();
  }

  void prepare(Sample normalizedCrossover) { lowpass_.prepare(normalizedCrossover); }

  void process(Sample x0) {
    output[0] = lowpass_.process(x0);
    output[1] = highpassDelay_.process(x0) - output[0];
  }
};

} // namespace Uhhyou
