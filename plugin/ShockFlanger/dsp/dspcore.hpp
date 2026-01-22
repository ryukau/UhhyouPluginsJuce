// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../parameter.hpp"
#include "Uhhyou/dsp/multirate.hpp"
#include "Uhhyou/dsp/smoother.hpp"
#include "fdn.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace Uhhyou {

class DSPCore {
private:
  using Real = double;

public:
  DSPCore(ParameterStore &param) : param(param) {}

  ParameterStore &param;
  bool isPlaying = false;
  Real tempo = Real(120);
  Real beatsElapsed = Real(0);
  Real timeSigUpper = Real(1);
  Real timeSigLower = Real(4);

  void setup(Real sampleRate);
  void reset();
  void startup();
  size_t getLatency();
  void setParameters();
  void process(
    const size_t length, const float *in0, const float *in1, float *out0, float *out1);

private:
  template<typename Func> void applyToParameters(Func apply);
  void updateUpRate();
  std::array<Real, 2> processSample(const std::array<Real, 2> in);

  static constexpr unsigned upFold = 2;
  unsigned overSampling = 1;
  Real sampleRate = 44100;
  Real upRate = upFold * 44100.0;

  static constexpr Real smootherTimeInSecond = Real(0.2);
  SmootherParameter<Real> smoo;
  Real fadeKp = 0;

  bool useFeedbackGate = false;
  Saturator<Real>::Function saturatorType = Saturator<Real>::Function::hardclip;

  ExpSmoother<Real> saturationGain{smoo};
  ExpSmoother<Real> feedback0{smoo};
  ExpSmoother<Real> feedback1{smoo};
  RotaryExpSmoother<Real> stereoPhaseOffset{smoo};
  ExpSmoother<Real> notchMix{smoo};
  ExpSmoother<Real> notchWidth{smoo};
  ExpSmoother<Real> notchTracking{smoo};
  ExpSmoother<Real> delayTimeSample0{smoo};
  ExpSmoother<Real> delayTimeSample1{smoo};
  ExpSmoother<Real> timeModOctave0{smoo};
  ExpSmoother<Real> timeModOctave1{smoo};
  ExpSmoother<Real> crossModOctave0{smoo};
  ExpSmoother<Real> crossModOctave1{smoo};
  ExpSmoother<Real> highpassCutoff{smoo};
  ExpSmootherLocal<Real> highpassFade;
  ExpSmoother<Real> flangeMode{smoo};
  ExpSmoother<Real> safeFeedback{smoo};
  ExpSmoother<Real> safeFlange{smoo};
  ExpSmoother<Real> lowpassCutoff{smoo};
  ExpSmootherLocal<Real> lowpassFade;
  ExpSmoother<Real> dryGain{smoo};
  ExpSmoother<Real> wetGain{smoo};

  TempoSyncedLfo<Real> lfo;
  std::array<Fdn2<Real>, 2> fdn;
  std::array<std::array<Real, 2>, 2> halfbandInput{};
  std::array<HalfBandIIR<Real, HalfBandCoefficient<Real>>, 2> halfbandIir;
};

} // namespace Uhhyou
