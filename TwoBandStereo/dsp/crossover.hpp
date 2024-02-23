// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cmath>
#include <complex>
#include <numbers>

namespace Uhhyou {

template<typename Sample, int length> class FixedIntDelay {
private:
  int ptr = 0;
  std::array<Sample, length> buf{};

public:
  void reset(Sample value = 0)
  {
    ptr = 0;
    buf.fill(value);
  }

  Sample process(Sample x0)
  {
    if (++ptr >= length) ptr = 0;
    Sample output = buf[ptr];
    buf[ptr] = x0;
    return output;
  }
};

template<typename Sample, size_t fullStage, size_t recStage> struct ComplexIIRDelay;

template<typename Sample, size_t fullStage> struct ComplexIIRDelay<Sample, fullStage, 0> {
  void reset() {}

  using C = std::complex<Sample>;
  inline C process1PoleForward(C x0, const std::array<C, fullStage> &) { return x0; }
  inline C process1PoleReversed(C x0, const std::array<C, fullStage> &) { return x0; }
};

template<typename Sample, size_t fullStage, size_t recStage> struct ComplexIIRDelay {
  static constexpr size_t index = fullStage - recStage;
  FixedIntDelay<std::complex<Sample>, size_t(1) << index> delay;
  ComplexIIRDelay<Sample, fullStage, recStage - 1> recursion;

  void reset()
  {
    delay.reset();
    recursion.reset();
  }

  inline std::complex<Sample> process1PoleForward(
    std::complex<Sample> x0, const std::array<std::complex<Sample>, fullStage> &poles)
  {
    return recursion.process1PoleForward(x0 + poles[index] * delay.process(x0), poles);
  }

  inline std::complex<Sample> process1PoleReversed(
    std::complex<Sample> x0, const std::array<std::complex<Sample>, fullStage> &poles)
  {
    return recursion.process1PoleReversed(poles[index] * x0 + delay.process(x0), poles);
  }
};

template<typename Sample, size_t stage = 8> class ComplexIIR {
private:
  static constexpr size_t nPoles = stage - 1;

  Sample a_per_b = 0;
  std::array<std::complex<Sample>, stage> poles;

  std::complex<Sample> x1 = 0;
  ComplexIIRDelay<Sample, stage, stage - 1> delay;

public:
  void reset()
  {
    x1 = 0;
    delay.reset();
  }

  void prepare(std::complex<Sample> pole)
  {
    a_per_b = pole.real() / pole.imag();
    for (auto &value : poles) {
      value = pole;
      pole *= pole;
    }
  }

  std::complex<Sample> process1PoleForward(Sample x0)
  {
    std::complex<Sample> sig = x0 + poles[0] * x1;
    x1 = x0;
    return delay.process1PoleForward(sig, poles);
  }

  std::complex<Sample> process1PoleReversed(Sample x0)
  {
    std::complex<Sample> sig = poles[0] * x0 + x1;
    x1 = x0;
    return delay.process1PoleReversed(sig, poles);
  }

  Sample process2PoleForward(Sample x0)
  {
    std::complex<Sample> sig = process1PoleForward(x0);
    return sig.real() + a_per_b * sig.imag();
  }

  Sample process2PoleReversed(Sample x0)
  {
    std::complex<Sample> sig = process1PoleReversed(x0);
    return sig.real() + a_per_b * sig.imag();
  }
};

template<typename Sample, size_t order, size_t stage = 8> class LinkwitzRileyFIR {
private:
  static constexpr size_t nSection = order / 4;

  std::array<ComplexIIR<Sample, stage>, nSection> reverse;
  std::array<ComplexIIR<Sample, stage>, nSection> forward;

  std::array<Sample, nSection> u1{};
  std::array<Sample, nSection> u2{};
  std::array<Sample, nSection> v1{};
  std::array<Sample, nSection> v2{};

  Sample gain = Sample(1);

public:
  static constexpr size_t latency = nSection * (size_t(1) << stage) + 1;

  LinkwitzRileyFIR() { static_assert(order % 4 == 0 && order >= 4); }

  void reset()
  {
    for (auto &x : reverse) x.reset();
    for (auto &x : forward) x.reset();

    u1.fill({});
    u2.fill({});
    v1.fill({});
    v2.fill({});
  }

  void prepare(Sample normalizedCrossover)
  {
    constexpr Sample pi = std::numbers::pi_v<Sample>;
    constexpr size_t N = 2 * nSection; // Butterworth order.

    gain = Sample(1);

    auto cutoffRadian = Sample(2) * pi * normalizedCrossover;
    for (size_t idx = 0; idx < nSection; ++idx) {
      auto m = Sample(2 * idx) - Sample(N) + Sample(1);
      auto analogPole = cutoffRadian * std::polar(Sample(-1), pi * m / Sample(2 * N));
      auto pole = (Sample(2) + analogPole) / (Sample(2) - analogPole);
      reverse[idx].prepare(pole);
      forward[idx].prepare(pole);
      gain *= (Sample(1) + Sample(-2) * pole.real() + std::norm(pole)) / Sample(4);
    }

    gain = std::pow(gain, Sample(1) / Sample(nSection));
  }

  Sample process(Sample x0)
  {
    constexpr Sample a1 = Sample(2); // -2 for highpass.

    for (size_t i = 0; i < nSection; ++i) {
      Sample u0 = reverse[i].process2PoleReversed(x0 * gain);
      x0 = u0 + a1 * u1[i] + u2[i];
      u2[i] = u1[i];
      u1[i] = u0;

      Sample v0 = forward[i].process2PoleForward(x0 * gain);
      x0 = v0 + a1 * v1[i] + v2[i];
      v2[i] = v1[i];
      v1[i] = v0;
    }
    return x0;
  }
};

template<typename Sample, size_t order, size_t stage = 8> class LinkwitzRileyFIR2Band4n {
public:
  static constexpr size_t latency = LinkwitzRileyFIR<Sample, order, stage>::latency;

private:
  LinkwitzRileyFIR<Sample, order, stage> lowpass;
  FixedIntDelay<Sample, latency> highpassDelay;

public:
  std::array<Sample, 2> output{}; // 0: low, 1: high.

  LinkwitzRileyFIR2Band4n() { static_assert(order % 4 == 0 && order >= 4); }

  size_t getLatency() { return latency; }

  void reset()
  {
    lowpass.reset();
    highpassDelay.reset();
  }

  void prepare(Sample normalizedCrossover) { lowpass.prepare(normalizedCrossover); }

  void process(Sample x0)
  {
    output[0] = lowpass.process(x0);
    output[1] = highpassDelay.process(x0) - output[0];
  }
};

} // namespace Uhhyou
