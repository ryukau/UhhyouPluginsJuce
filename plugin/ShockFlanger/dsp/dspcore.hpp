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
public:
  using Real = double;

  DSPCore(ParameterStore& param) : param(param) { noteIdStack.reserve(1024); }

  ParameterStore& param;
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
  void process(const size_t length, const float* in0, const float* in1, float* out0, float* out1);

  void noteOn(int noteId, Real pitchSemitone, Real velocity);
  void noteOff(int noteId);
  void notePitchBend(int noteId, Real bend);
  void setPitchBend(Real bend);

private:
  template<typename Func> void applyToParameters(Func apply);
  void updateUpRate();
  std::array<Real, 2> processSample(const std::array<Real, 2> in);

  static constexpr unsigned upFold = 2;
  unsigned overSampling = 1;
  Real sampleRate = 44100;
  Real upRate = upFold * 44100.0;

  bool isResettingLfoPhase = false;
  bool useFeedbackGate = false;
  bool noteReceive = false;

  static constexpr Real smootherTimeInSecond = Real(0.2);
  SmootherParameter<Real> smoo;
  Real fadeKp = 0;
  Real noteKp = 0;

  Saturator<Real>::Function saturatorType = Saturator<Real>::Function::hardclip;

  struct NoteData {
    int noteId{};
    Real pitch{};
    Real gain{};
  };
  Real notePitchScalar = Real(1);
  Real noteGainScalar = Real(1);
  std::vector<NoteData> noteIdStack;
  ExpSmootherLocal<Real> notePitch;
  ExpSmootherLocal<Real> noteGain;
  ExpSmootherLocal<Real> globalPitchBend;

  ExpSmoother<Real> saturationGain{smoo};
  ExpSmoother<Real> inputRatio{smoo};
  ExpSmoother<Real> feedback0{smoo};
  ExpSmoother<Real> feedback1{smoo};
  RotaryExpSmoother<Real> lfoPhaseInitial{smoo};
  RotaryExpSmoother<Real> lfoPhaseStereoOffset{smoo};
  ExpSmoother<Real> delayTimeSample0{smoo};
  ExpSmoother<Real> delayTimeSample1{smoo};
  ExpSmoother<Real> crossModMode{smoo};
  ExpSmoother<Real> crossModOctave0{smoo};
  ExpSmoother<Real> crossModOctave1{smoo};
  ExpSmoother<Real> timeModOctave0{smoo};
  ExpSmoother<Real> timeModOctave1{smoo};
  ExpSmoother<Real> highpassCutoff{smoo};
  ExpSmootherLocal<Real> highpassFade;
  ExpSmoother<Real> flangeMode{smoo};
  ExpSmoother<Real> safeFeedback{smoo};
  ExpSmoother<Real> safeFlange{smoo};
  ExpSmoother<Real> lowpassCutoff{smoo};
  ExpSmootherLocal<Real> lowpassFade;
  ExpSmoother<Real> dryGain{smoo};
  ExpSmoother<Real> wetGain{smoo};

  std::array<Real, 2> preSaturationPeak{};
  std::array<Real, 2> outputPeak{};
  std::array<Real, 2> modPhase{};
  std::array<Fdn2<Real>::DisplayTime, 2> displayTime;
  TempoSyncedLfo<Real> lfo;
  std::array<Fdn2<Real>, 2> fdn;
  std::array<std::array<Real, 2>, 2> halfbandInput{};
  std::array<HalfBandIIR<Real, HalfBandCoefficient<Real>>, 2> halfbandIir;
};

} // namespace Uhhyou
