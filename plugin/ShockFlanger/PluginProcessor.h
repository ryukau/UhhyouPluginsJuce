// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

#include "dsp/dspcore.hpp"
#include "parameter.hpp"

#include <mutex>

class Processor final : public juce::AudioProcessor,
                        public juce::MPEInstrument::Listener {
public:
  Processor();
  ~Processor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  virtual void noteAdded(juce::MPENote newNote) override;
  virtual void notePressureChanged(juce::MPENote changedNote) override;
  virtual void notePitchbendChanged(juce::MPENote changedNote) override;
  virtual void noteTimbreChanged(juce::MPENote changedNote) override;
  virtual void noteKeyStateChanged(juce::MPENote changedNote) override;
  virtual void noteReleased(juce::MPENote finishedNote) override;
  virtual void zoneLayoutChanged() override;

public:
  int midiSampleOffset = 0;
  juce::MPEInstrument mpeInstrument;
  juce::UndoManager undoManager{32768, 512};

  Uhhyou::ParameterStore param;
  Uhhyou::DSPCore dsp;
  double previousSampleRate = double(-1);

private:
  std::mutex setupMutex;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};
