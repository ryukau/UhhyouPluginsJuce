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
  Real tempo = Real(120);
  Real beatsElapsed = Real(0);
  Real timeSigUpper = Real(1);
  Real timeSigLower = Real(4);
  bool isPlaying = false;

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
  static constexpr Real smootherTimeInSecond = Real(0.2);

  struct NoteData {
    Real pitch{};
    Real gain{};
    int noteId{};
  };

  std::vector<NoteData> noteIdStack;

  Real sampleRate = 44100;
  Real upRate = upFold * 44100.0;
  Real fadeKp = 0;
  Real noteKp = 0;
  Real notePitchScalar = Real(1);
  Real noteGainScalar = Real(1);

  unsigned overSampling = 1;
  Saturator<Real>::Function saturatorType = Saturator<Real>::Function::hardclip;

  bool isResettingLfoPhase = false;
  bool useFeedbackGate = false;
  bool noteReceive = false;

  SmootherParameter<Real> smoo;
  ExpSmootherLocal<Real> notePitch;
  ExpSmootherLocal<Real> noteGain;
  ExpSmootherLocal<Real> globalPitchBend;

  ExpSmoother<Real> saturationGain{smoo};
  ExpSmoother<Real> inputBlend{smoo};
  ExpSmoother<Real> feedback0{smoo};
  ExpSmoother<Real> feedback1{smoo};
  RotaryExpSmoother<Real> lfoPhaseInitial{smoo};
  RotaryExpSmoother<Real> lfoPhaseStereoOffset{smoo};
  ExpSmoother<Real> delayTimeSample0{smoo};
  ExpSmoother<Real> delayTimeSample1{smoo};
  ExpSmoother<Real> viscosityCutoff{smoo};
  ExpSmoother<Real> sigModMode{smoo};
  ExpSmoother<Real> sigTimeMod0{smoo};
  ExpSmoother<Real> sigTimeMod1{smoo};
  ExpSmoother<Real> lfoTimeMod0{smoo};
  ExpSmoother<Real> lfoTimeMod1{smoo};
  ExpSmoother<Real> sigAmpMod0{smoo};
  ExpSmoother<Real> sigAmpMod1{smoo};
  ExpSmoother<Real> highpassCutoff{smoo};
  ExpSmootherLocal<Real> highpassFade;
  ExpSmoother<Real> flangeBlend{smoo};
  ExpSmoother<Real> safeFeedback{smoo};
  ExpSmoother<Real> flangePolarity{smoo};
  ExpSmoother<Real> lowpassCutoff{smoo};
  ExpSmootherLocal<Real> lowpassFade;
  ExpSmoother<Real> dryGain{smoo};
  ExpSmoother<Real> wetGain{smoo};

  std::array<Real, 2> preSaturationPeak{};
  std::array<Real, 2> outputPeak{};
  std::array<Real, 2> modPhase{};
  std::array<Fdn2<Real>::DisplayTime, 2> displayTime;
  std::array<std::array<Real, 2>, 2> halfbandInput{};
  TempoSyncedLfo<Real> lfo;
  std::array<HalfBandIIR<Real, HalfBandCoefficient<Real>>, 2> halfbandIir;
  std::array<Fdn2<Real>, 2> fdn;
};

} // namespace Uhhyou
