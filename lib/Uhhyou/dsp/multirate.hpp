// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "multiratecoefficient.hpp"

#include <algorithm>

namespace Uhhyou {

template<typename Sample, typename Sos> class DecimationLowpass {
  struct State {
    Sample s1 = 0;
    Sample s2 = 0;
  };
  alignas(64) std::array<State, Sos::co.size()> states{};
  Sample last = 0;

public:
  void reset()
  {
    states.fill({});
    last = 0;
  }

  inline void push(Sample x)
  {
    for (size_t i = 0; i < Sos::co.size(); ++i) {
      const auto &c = Sos::co[i];
      Sample y = c[0] * x + states[i].s1;
      states[i].s1 = states[i].s2 + c[1] * x - c[3] * y;
      states[i].s2 = c[2] * x - c[4] * y;
      x = y;
    }
    last = x;
  }

  inline Sample output() const { return last; }
};

template<typename Sample, const auto &Coefficients> class AllpassCascade {
private:
  std::array<Sample, Coefficients.size()> s{};

  template<std::size_t... I>
  inline Sample process_impl(Sample input, std::index_sequence<I...>)
  {
    ((input = process_section<I>(input)), ...);
    return input;
  }

  template<std::size_t Index> inline Sample process_section(Sample input)
  {
    constexpr auto coeff = Coefficients[Index];
    Sample y = coeff * input + s[Index];
    s[Index] = input - coeff * y;
    return y;
  }

public:
  void reset() { s.fill(Sample(0)); }

  inline Sample process(Sample input)
  {
    return process_impl(input, std::make_index_sequence<Coefficients.size()>{});
  }
};

template<typename Sample, typename Coefficient> class HalfBandIIR {
private:
  AllpassCascade<Sample, Coefficient::h0_a> ap0;
  AllpassCascade<Sample, Coefficient::h1_a> ap1;

public:
  void reset()
  {
    ap0.reset();
    ap1.reset();
  }

  // For down-sampling. input[0] is earlier sample (z^-1).
  inline Sample process(const std::array<Sample, 2> &input)
  {
    auto s0 = ap0.process(input[0]);
    auto s1 = ap1.process(input[1]);
    return Sample(0.5) * (s0 + s1);
  }

  // For up-sampling.
  std::array<Sample, 2> processUp(Sample input)
  {
    return {ap1.process(input), ap0.process(input)};
  }
};

template<typename Sample, typename FractionalDelayFIR> class FirUpSampler {
  std::array<Sample, FractionalDelayFIR::bufferSize> buf{};

public:
  std::array<Sample, FractionalDelayFIR::upfold> output;

  void reset() { buf.fill(Sample(0)); }

  void process(Sample input)
  {
    std::rotate(buf.rbegin(), buf.rbegin() + 1, buf.rend());
    buf[0] = input;

    std::fill(output.begin(), output.end(), Sample(0));
    for (size_t i = 0; i < FractionalDelayFIR::coefficient.size(); ++i) {
      auto &&phase = FractionalDelayFIR::coefficient[i];
      for (size_t n = 0; n < phase.size(); ++n) output[i] += buf[n] * phase[n];
    }
  }
};

template<typename Sample, typename FractionalDelayFIR> class TruePeakMeterConvolver {
  std::array<Sample, FractionalDelayFIR::bufferSize> buf{};

public:
  std::array<Sample, FractionalDelayFIR::upfold> output;

  void reset() { buf.fill(Sample(0)); }

  void process(Sample input)
  {
    std::rotate(buf.rbegin(), buf.rbegin() + 1, buf.rend());
    buf[0] = input;

    std::fill(output.begin(), output.end() - 1, Sample(0));
    output.back() = buf[FractionalDelayFIR::intDelay];
    for (size_t i = 0; i < FractionalDelayFIR::coefficient.size(); ++i) {
      auto &&phase = FractionalDelayFIR::coefficient[i];
      for (size_t n = 0; n < phase.size(); ++n) output[i] += buf[n] * phase[n];
    }
  }
};

template<typename Sample, size_t upSample> class CubicUpSampler {
  std::array<Sample, 4> buf{};

public:
  std::array<Sample, upSample> output;

  void reset() { buf.fill(Sample(0)); }

  // 3rd order Lagrange Interpolation.
  // Range of t is in [0, 1]. Interpolates between y1 and y2.
  inline Sample cubicInterp(const std::array<Sample, 4> &y, Sample t)
  {
    auto u = 1 + t;
    auto d0 = y[0] - y[1];
    auto d1 = d0 - (y[1] - y[2]);
    auto d2 = d1 - ((y[1] - y[2]) - (y[2] - y[3]));
    return y[0] - ((d2 * (2 - u) / 3 + d1) * (1 - u) / 2 + d0) * u;
  }

  void process(Sample input)
  {
    std::rotate(buf.begin(), buf.begin() + 1, buf.end());
    buf.back() = input;

    std::fill(output.begin() + 1, output.end(), Sample(0));
    output[0] = buf[1];
    for (size_t i = 1; i < output.size(); ++i) {
      output[i] = cubicInterp(buf, Sample(i) / Sample(upSample));
    }
  }
};

template<typename Sample, size_t upSample> class LinearUpSampler {
  Sample buf = 0;

public:
  std::array<Sample, upSample> output;

  void reset() { buf = 0; }

  void process(Sample input)
  {
    Sample diff = input - buf;
    for (size_t i = 0; i < output.size(); ++i) {
      output[i] = buf + diff * Sample(i) / Sample(upSample);
    }
    buf = input;
  }
};

template<typename Sample> struct OverSampler16 {
  static constexpr size_t fold = 16;

  FirUpSampler<Sample, Fir16FoldUpSample<Sample>> upSampler;
  std::array<Sample, 16> inputBuffer{};
  DecimationLowpass<Sample, Sos16FoldFirstStage<Sample>> lowpass;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> halfbandIir;

  void reset()
  {
    upSampler.reset();
    inputBuffer.fill({});
    lowpass.reset();
    halfbandIir.reset();
  }

  void push(Sample x0) { upSampler.process(x0); }
  Sample at(size_t index) { return upSampler.output[index]; }

  Sample process()
  {
    std::array<Sample, 2> halfBandInput;
    for (size_t i = 0; i < 8; ++i) lowpass.push(inputBuffer[i]);
    halfBandInput[0] = lowpass.output();
    for (size_t i = 8; i < 16; ++i) lowpass.push(inputBuffer[i]);
    halfBandInput[1] = lowpass.output();
    return halfbandIir.process(halfBandInput);
  }
};

template<typename Sample, typename FirstStageSosCoefficient> struct DownSampler {
  static constexpr size_t fold = 2 * FirstStageSosCoefficient::fold;

  std::array<Sample, fold> inputBuffer{};
  DecimationLowpass<Sample, FirstStageSosCoefficient> lowpass;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> halfbandIir;

  void reset()
  {
    inputBuffer.fill({});
    lowpass.reset();
    halfbandIir.reset();
  }

  Sample process()
  {
    std::array<Sample, 2> halfBandInput{};
    for (size_t i = 0; i < fold / 2; ++i) lowpass.push(inputBuffer[i]);
    halfBandInput[0] = lowpass.output();
    for (size_t i = fold / 2; i < fold; ++i) lowpass.push(inputBuffer[i]);
    halfBandInput[1] = lowpass.output();
    return halfbandIir.process(halfBandInput);
  }

  Sample process2x() { return halfbandIir.process({inputBuffer[0], inputBuffer[1]}); }
};

} // namespace Uhhyou
