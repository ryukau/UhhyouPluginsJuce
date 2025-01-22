// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "multiratecoefficient.hpp"

#include <algorithm>

namespace Uhhyou {

template<typename Sample, typename Sos> class DecimationLowpass {
  std::array<Sample, Sos::co.size()> x0{};
  std::array<Sample, Sos::co.size()> x1{};
  std::array<Sample, Sos::co.size()> x2{};
  std::array<Sample, Sos::co.size()> y0{};
  std::array<Sample, Sos::co.size()> y1{};
  std::array<Sample, Sos::co.size()> y2{};

public:
  void reset()
  {
    x0.fill(0);
    x1.fill(0);
    x2.fill(0);
    y0.fill(0);
    y1.fill(0);
    y2.fill(0);
  }

  void push(Sample input) noexcept
  {
    x0[0] = input;
    std::copy(y0.begin(), y0.end() - 1, x0.begin() + 1);

    for (size_t i = 0; i < Sos::co.size(); ++i) {
      y0[i]                      //
        = Sos::co[i][0] * x0[i]  //
        + Sos::co[i][1] * x1[i]  //
        + Sos::co[i][2] * x2[i]  //
        - Sos::co[i][3] * y1[i]  //
        - Sos::co[i][4] * y2[i]; //
    }

    x2 = x1;
    x1 = x0;
    y2 = y1;
    y1 = y0;
  }

  inline Sample output() { return y0[Sos::co.size() - 1]; }
};

template<typename Sample, size_t nSection> class FirstOrderAllpassSections {
private:
  std::array<Sample, nSection> x{};
  std::array<Sample, nSection> y{};

public:
  void reset()
  {
    x.fill(0);
    y.fill(0);
  }

  Sample process(Sample input, const std::array<Sample, nSection> &a)
  {
    for (size_t i = 0; i < nSection; ++i) {
      y[i] = a[i] * (input - y[i]) + x[i];
      x[i] = input;
      input = y[i];
    }
    return y.back();
  }
};

template<typename Sample, typename Coefficient> class HalfBandIIR {
private:
  FirstOrderAllpassSections<Sample, Coefficient::h0_a.size()> ap0;
  FirstOrderAllpassSections<Sample, Coefficient::h1_a.size()> ap1;

public:
  void reset()
  {
    ap0.reset();
    ap1.reset();
  }

  // For down-sampling. input[0] must be earlier sample.
  Sample process(const std::array<Sample, 2> &input)
  {
    auto s0 = ap0.process(input[0], Coefficient::h0_a);
    auto s1 = ap1.process(input[1], Coefficient::h1_a);
    return Sample(0.5) * (s0 + s1);
  }

  // For up-sampling.
  std::array<Sample, 2> processUp(Sample input)
  {
    return {
      ap1.process(input, Coefficient::h1_a),
      ap0.process(input, Coefficient::h0_a),
    };
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
