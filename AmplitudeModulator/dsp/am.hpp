// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../common/dsp/multirate.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>

namespace Uhhyou {

// `Sos` is filter coefficients in second order sections (sos) format. The format is
// similar to SciPy's sos, but the first 1 in denominator (typically denoted as $a_0$)
// is omitted.
template<typename Sample, typename Sos> class SosFilter {
  std::array<Sample, Sos::co.size()> x1{};
  std::array<Sample, Sos::co.size()> x2{};
  std::array<Sample, Sos::co.size()> y1{};
  std::array<Sample, Sos::co.size()> y2{};

public:
  void reset()
  {
    x1.fill(0);
    x2.fill(0);
    y1.fill(0);
    y2.fill(0);
  }

  Sample process(Sample input)
  {
    for (size_t i = 0; i < Sos::co.size(); ++i) {
      Sample y0                  //
        = Sos::co[i][0] * input  //
        + Sos::co[i][1] * x1[i]  //
        + Sos::co[i][2] * x2[i]  //
        - Sos::co[i][3] * y1[i]  //
        - Sos::co[i][4] * y2[i]; //

      x2[i] = x1[i];
      x1[i] = input;
      y2[i] = y1[i];
      y1[i] = y0;

      input = y0;
    }
    return input;
  }
};

// ```pytyon
// import scipy.signal as signal
// sos = signal.ellip(16, 0.01, 140, 0.925 / 6, "lowpass", output="sos", fs=1)
// ```
template<typename Sample> class SosOneThirdLowpass {
public:
  constexpr static std::array<std::array<Sample, 5>, 8> co{{
    {Sample(1.4299700336859399e-05), Sample(2.6223643283408427e-05),
     Sample(1.4299700336859402e-05), Sample(-1.419010779886042),
     Sample(0.5152639120776978)},
    {Sample(1.0000000000000000), Sample(0.9374859430410645), Sample(1.0000000000000000),
     Sample(-1.374108854632666), Sample(0.569988993082886)},
    {Sample(1.0000000000000000), Sample(0.1350973292280386), Sample(0.9999999999999998),
     Sample(-1.3033222470303198), Sample(0.6571339849004512)},
    {Sample(1.0000000000000000), Sample(-0.3463388380253702), Sample(1.0000000000000002),
     Sample(-1.2300429244541804), Sample(0.7495761953198858)},
    {Sample(1.0000000000000000), Sample(-0.6140989162812137), Sample(1.0000000000000000),
     Sample(-1.1698275005007623), Sample(0.8300611494196282)},
    {Sample(1.0000000000000000), Sample(-0.7615991375114761), Sample(1.0000000000000002),
     Sample(-1.1287956505319012), Sample(0.8934017825312789)},
    {Sample(1.0000000000000000), Sample(-0.83977959367167), Sample(1.0000000000000002),
     Sample(-1.1078055040479997), Sample(0.9420069143074725)},
    {Sample(1.0000000000000000), Sample(-0.87372678641345), Sample(1.0000000000000002),
     Sample(-1.1067875902296604), Sample(0.9815369953292316)},
  }};
};

// ```pytyon
// import scipy.signal as signal
// # Highpass stopband attenuation is -60 dB to achieve sharper fall off.
// lowCut = (1 + 60 / 48000) / 6
// highCut = 1.925 / 6
// sosBp = np.vstack(
//     [
//         signal.ellip(16, 0.01, 60, lowCut, "highpass", output="sos", fs=1),
//         signal.ellip(16, 0.01, 140, highCut, "lowpass", output="sos", fs=1),
//     ]
// )
// ```
template<typename Sample> class SosOneThirdBandpass {
public:
  constexpr static std::array<std::array<Sample, 5>, 16> co{{
    {Sample(0.07996306031918912), Sample(-0.15512096495683905),
     Sample(0.07996306031918912), Sample(-0.04905756091899954),
     Sample(0.06089317414996781)},
    {Sample(1.0000000000000000), Sample(-1.605912361850232), Sample(1.0000000000000000),
     Sample(-0.3914194433835788), Sample(0.39780395780767436)},
    {Sample(1.0000000000000000), Sample(-1.3044450264840837), Sample(1.0000000000000000),
     Sample(-0.69851818886442), Sample(0.7000220928086291)},
    {Sample(1.0000000000000000), Sample(-1.1386205501393927), Sample(0.9999999999999999),
     Sample(-0.8666910823954345), Sample(0.8655475347010958)},
    {Sample(1.0000000000000000), Sample(-1.0617196394441732), Sample(1.0000000000000002),
     Sample(-0.9445579714023477), Sample(0.9422516172059531)},
    {Sample(1.0000000000000000), Sample(-1.0283402094063119), Sample(1.0000000000000002),
     Sample(-0.9784715696658016), Sample(0.9758114926561893)},
    {Sample(1.0000000000000000), Sample(-1.0144993240335434), Sample(1.0000000000000000),
     Sample(-0.9928926526136225), Sample(0.990449173400009)},
    {Sample(1.0000000000000000), Sample(-1.0095384322002112), Sample(1.0000000000000000),
     Sample(-0.998957500114774), Sample(0.9974851593731303)},
    {Sample(0.003929281896699457), Sample(0.007783864623170485),
     Sample(0.003929281896699457), Sample(-0.6125926872786202),
     Sample(0.13187643733994786)},
    {Sample(1.0000000000000000), Sample(1.8466736192584305), Sample(1.0000000000000000),
     Sample(-0.3190661665817673), Sample(0.29140967523377553)},
    {Sample(1.0000000000000000), Sample(1.648795055047372), Sample(0.9999999999999998),
     Sample(0.06076167121153805), Sample(0.4984243325343967)},
    {Sample(1.0000000000000000), Sample(1.459139853427764), Sample(0.9999999999999998),
     Sample(0.37703985322489586), Sample(0.6721138263458347)},
    {Sample(1.0000000000000000), Sample(1.3116532086335044), Sample(0.9999999999999999),
     Sample(0.5973914775512698), Sample(0.7955385873005747)},
    {Sample(1.0000000000000000), Sample(1.2109484406557494), Sample(1.0000000000000000),
     Sample(0.7387197862035764), Sample(0.8788571064087001)},
    {Sample(1.0000000000000000), Sample(1.1502952430178814), Sample(0.9999999999999998),
     Sample(0.824163636062829), Sample(0.9362950931513307)},
    {Sample(1.0000000000000000), Sample(1.1221376470808058), Sample(0.9999999999999999),
     Sample(0.8708683205996026), Sample(0.980025035115969)},
  }};
};

// ```pytyon
// import scipy.signal as signal
// cutoff = (1 + 60 / 48000) / 4
// sos = signal.ellip(16, 0.01, 60, cutoff, "highpass", output="sos", fs=1)
// ```
template<typename Sample> class SosHalfHighpass {
public:
  constexpr static std::array<std::array<Sample, 5>, 8> co{{
    {Sample(0.022533030123118865), Sample(-0.0411189601921383),
     Sample(0.022533030123118865), Sample(0.5209635159310878),
     Sample(0.1207066785590715)},
    {Sample(1.0), Sample(-1.011674456291376), Sample(1.0000000000000002),
     Sample(0.3573911427742562), Sample(0.39490425738945695)},
    {Sample(1.0), Sample(-0.45091438753049795), Sample(0.9999999999999999),
     Sample(0.188876540910701), Sample(0.677405647200568)},
    {Sample(1.0), Sample(-0.19288475235068764), Sample(1.0), Sample(0.08617820982686307),
     Sample(0.8496269018092625)},
    {Sample(1.0), Sample(-0.08311788874077465), Sample(1.0000000000000002),
     Sample(0.03583311445085322), Sample(0.9341951501489619)},
    {Sample(1.0), Sample(-0.03724249975861203), Sample(0.9999999999999999),
     Sample(0.013408644836165261), Sample(0.972208991199606)},
    {Sample(1.0), Sample(-0.018521267756271977), Sample(1.0000000000000002),
     Sample(0.004004687637778214), Sample(0.9889883586768273)},
    {Sample(1.0), Sample(-0.011853373888226475), Sample(1.0),
     Sample(0.0006172958500284454), Sample(0.9970964690788071)},
  }};
};

template<typename Sample> class AnalyticSignalFilter {
private:
  constexpr static std::array<Sample, 4> coRe{
    Sample(0.16175849836770106), Sample(0.7330289323414905), Sample(0.9453497003291133),
    Sample(0.9905991566845292)};
  constexpr static std::array<Sample, 4> coIm{
    Sample(0.47940086558884), Sample(0.8762184935393101), Sample(0.9765975895081993),
    Sample(0.9974992559355491)};

  std::array<Sample, coRe.size()> x1Re{};
  std::array<Sample, coRe.size()> x2Re{};
  std::array<Sample, coRe.size()> y1Re{};
  std::array<Sample, coRe.size()> y2Re{};

  std::array<Sample, coIm.size()> x1Im{};
  std::array<Sample, coIm.size()> x2Im{};
  std::array<Sample, coIm.size()> y1Im{};
  std::array<Sample, coIm.size()> y2Im{};

  Sample delayedIm = 0;

public:
  void reset()
  {
    x1Re.fill(0);
    x2Re.fill(0);
    y1Re.fill(0);
    y2Re.fill(0);

    x1Im.fill(0);
    x2Im.fill(0);
    y1Im.fill(0);
    y2Im.fill(0);

    delayedIm = 0;
  }

  std::complex<Sample> process(Sample input)
  {
    auto sigRe = input;
    for (size_t i = 0; i < coRe.size(); ++i) {
      auto y0 = coRe[i] * (sigRe + y2Re[i]) - x2Re[i];
      x2Re[i] = x1Re[i];
      x1Re[i] = sigRe;
      y2Re[i] = y1Re[i];
      y1Re[i] = y0;
      sigRe = y0;
    }

    auto sigIm = input;
    for (size_t i = 0; i < coIm.size(); ++i) {
      auto y0 = coIm[i] * (sigIm + y2Im[i]) - x2Im[i];
      x2Im[i] = x1Im[i];
      x1Im[i] = sigIm;
      y2Im[i] = y1Im[i];
      y1Im[i] = y0;
      sigIm = y0;
    }
    auto outIm = delayedIm;
    delayedIm = sigIm; // 1 sample delay.

    return std::complex<Sample>{sigRe, outIm};
  }
};

template<typename Sample> class FrequencyShifter {
private:
  AnalyticSignalFilter<Sample> hilbert;
  Sample phase = 0;

public:
  void reset()
  {
    hilbert.reset();
    phase = 0;
  }

  // `shiftFreq` is normalized frequency in [0, 0.5).
  Sample process(Sample input, Sample shiftFreq)
  {
    auto sig = hilbert.process(input);
    auto norm = std::sqrt(sig.real() * sig.real() + sig.imag() * sig.imag());
    auto theta = std::atan2(sig.imag(), sig.real());

    phase += shiftFreq;
    phase -= std::floor(phase);
    return norm * std::cos(theta + Sample(2 * std::numbers::pi) * phase);
  }
};

// This class is made to shorten the code in `dspcore.cpp`. It's just a multiplication.
template<typename Sample> class AmplitudeModulator {
public:
  void reset() {}
  Sample process(Sample carrior, Sample modulator) { return carrior * modulator; }
};

template<typename Sample> class AmplitudeModulatorUpperAA {
private:
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbCar;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbMod;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbDown;

public:
  void reset()
  {
    hbCar.reset();
    hbMod.reset();
    hbDown.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    auto upCar = hbCar.processUp(carrior);
    auto upMod = hbMod.processUp(modulator);
    return hbDown.process({upCar[0] * upMod[0], upCar[1] * upMod[1]});
  }
};

template<typename Sample> class AmplitudeModulatorFullAA {
private:
  SosFilter<Sample, SosOneThirdLowpass<Sample>> lowpassCar;
  SosFilter<Sample, SosOneThirdLowpass<Sample>> lowpassMod;
  SosFilter<Sample, SosOneThirdLowpass<Sample>> lowpassDown;
  SosFilter<Sample, SosOneThirdBandpass<Sample>> bandpassAm;
  FrequencyShifter<Sample> forwardShifter;
  FrequencyShifter<Sample> backwardShifter;

public:
  void reset()
  {
    lowpassCar.reset();
    lowpassMod.reset();
    lowpassDown.reset();
    bandpassAm.reset();
    forwardShifter.reset();
    backwardShifter.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    Sample output;
    for (size_t idx = 0; idx < 3; ++idx) {
      carrior = lowpassCar.process(carrior);
      modulator = lowpassMod.process(modulator);

      constexpr Sample shiftFreq = Sample(1) / Sample(6);
      auto am = modulator * forwardShifter.process(carrior, shiftFreq);
      auto filtered = bandpassAm.process(am);
      auto result = backwardShifter.process(filtered, -shiftFreq);

      carrior = Sample(0);
      modulator = Sample(0);
      output = lowpassDown.process(result);
    }

    // Multiply by 9 comes from `(3-fold expanded carrior) * (3-fold expanded modulator)`.
    return Sample(9) * output;
  }
};

template<typename Sample> class UpperSideBandAmplitudeModulator {
private:
  AnalyticSignalFilter<Sample> carFilter;
  AnalyticSignalFilter<Sample> modFilter;

public:
  void reset()
  {
    carFilter.reset();
    modFilter.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    auto c0 = carFilter.process(carrior);
    auto m0 = modFilter.process(modulator);
    return c0.real() * m0.real() - c0.imag() * m0.imag();
  }
};

// Almost identical to `UpperSideBandAmplitudeModulator`. Splitted to shorten the code in
// `dspcore.cpp`.
template<typename Sample> class LowerSideBandAmplitudeModulator {
private:
  AnalyticSignalFilter<Sample> carFilter;
  AnalyticSignalFilter<Sample> modFilter;

public:
  void reset()
  {
    carFilter.reset();
    modFilter.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    auto c0 = carFilter.process(carrior);
    auto m0 = modFilter.process(modulator);
    return c0.real() * m0.real() + c0.imag() * m0.imag();
  }
};

template<typename Sample> class UpperSideBandAmplitudeModulatorAA {
private:
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbCar;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbMod;
  UpperSideBandAmplitudeModulator<Sample> usb;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbDown;

public:
  void reset()
  {
    hbCar.reset();
    hbMod.reset();
    usb.reset();
    hbDown.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    auto upCar = hbCar.processUp(carrior);
    auto upMod = hbMod.processUp(modulator);
    auto s0 = usb.process(upCar[0], upMod[0]);
    auto s1 = usb.process(upCar[1], upMod[1]);
    return hbDown.process({s0, s1});
  }
};

template<typename Sample> class LowerSideBandAmplitudeModulatorAA {
private:
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbCar;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbMod;
  HalfBandIIR<Sample, HalfBandCoefficient<Sample>> hbDown;
  FrequencyShifter<Sample> forwardShifter;
  LowerSideBandAmplitudeModulator<Sample> lsb;
  SosFilter<Sample, SosHalfHighpass<Sample>> highpass;
  FrequencyShifter<Sample> backwardShifter;

public:
  void reset()
  {
    hbCar.reset();
    hbMod.reset();
    hbDown.reset();
    forwardShifter.reset();
    lsb.reset();
    highpass.reset();
    backwardShifter.reset();
  }

  Sample process(Sample carrior, Sample modulator)
  {
    auto upCar = hbCar.processUp(carrior);
    auto upMod = hbMod.processUp(modulator);
    std::array<Sample, 2> output{};
    for (size_t idx = 0; idx < 2; ++idx) {
      constexpr Sample shiftFreq = Sample(0.25);
      auto shiftedCar = forwardShifter.process(upCar[idx], shiftFreq);
      auto am = lsb.process(shiftedCar, upMod[idx]);
      auto filtered = highpass.process(am);
      output[idx] = backwardShifter.process(filtered, -shiftFreq);
    }
    return hbDown.process(output);
  }
};

} // namespace Uhhyou
