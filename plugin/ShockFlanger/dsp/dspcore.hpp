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

  DSPCore(ParameterStore& p) : param(p) { noteIdStack_.reserve(1024); }

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

  std::vector<NoteData> noteIdStack_;

  Real sampleRate_ = 44100;
  Real upRate_ = upFold * 44100.0;
  Real fadeKp_ = 0;
  Real noteKp_ = 0;
  Real notePitchScalar_ = Real(1);
  Real noteGainScalar_ = Real(1);

  unsigned overSampling_ = 1;
  Saturator<Real>::Function saturatorType_ = Saturator<Real>::Function::hardclip;

  bool isResettingLfoPhase_ = false;
  bool useFeedbackGate_ = false;
  bool noteReceive_ = false;

  SmootherParameter<Real> smoo_;
  ExpSmootherLocal<Real> notePitch_;
  ExpSmootherLocal<Real> noteGain_;
  ExpSmootherLocal<Real> globalPitchBend_;

  ExpSmoother<Real> saturationGain_{smoo_};
  ExpSmoother<Real> inputBlend_{smoo_};
  ExpSmoother<Real> feedback0_{smoo_};
  ExpSmoother<Real> feedback1_{smoo_};
  RotaryExpSmoother<Real> lfoPhaseInitial_{smoo_};
  RotaryExpSmoother<Real> lfoPhaseStereoOffset_{smoo_};
  ExpSmoother<Real> delayTimeSample0_{smoo_};
  ExpSmoother<Real> delayTimeSample1_{smoo_};
  ExpSmoother<Real> viscosityCutoff_{smoo_};
  ExpSmoother<Real> sigModMode_{smoo_};
  ExpSmoother<Real> sigTimeMod0_{smoo_};
  ExpSmoother<Real> sigTimeMod1_{smoo_};
  ExpSmoother<Real> lfoTimeMod0_{smoo_};
  ExpSmoother<Real> lfoTimeMod1_{smoo_};
  ExpSmoother<Real> sigAmpMod0_{smoo_};
  ExpSmoother<Real> sigAmpMod1_{smoo_};
  ExpSmoother<Real> highpassCutoff_{smoo_};
  ExpSmootherLocal<Real> highpassFade_;
  ExpSmoother<Real> flangeBlend_{smoo_};
  ExpSmoother<Real> safeFeedback_{smoo_};
  ExpSmoother<Real> flangePolarity_{smoo_};
  ExpSmoother<Real> lowpassCutoff_{smoo_};
  ExpSmootherLocal<Real> lowpassFade_;
  ExpSmoother<Real> dryGain_{smoo_};
  ExpSmoother<Real> wetGain_{smoo_};

  std::array<Real, 2> preSaturationPeak_{};
  std::array<Real, 2> outputPeak_{};
  std::array<Real, 2> modPhase_{};
  std::array<Fdn2<Real>::DisplayTime, 2> displayTime_;
  std::array<std::array<Real, 2>, 2> halfbandInput_{};
  TempoSyncedLfo<Real> lfo_;
  std::array<HalfBandIIR<Real, HalfBandCoefficient<Real>>, 2> halfbandIir_;
  std::array<Fdn2<Real>, 2> fdn_;
};

} // namespace Uhhyou
