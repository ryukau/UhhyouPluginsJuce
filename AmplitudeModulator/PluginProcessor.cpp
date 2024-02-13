// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

Processor::Processor()
  : AudioProcessor(BusesProperties()
                     .withInput("InputCarrior", juce::AudioChannelSet::stereo(), true)
                     .withInput("InputModulator", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
  , param(*this, &undoManager, juce::Identifier("Root"))
  , dsp(param)
{
  mpeInstrument.addListener(this);
}

Processor::~Processor() {}

const juce::String Processor::getName() const { return JucePlugin_Name; }
bool Processor::acceptsMidi() const { return true; }
bool Processor::producesMidi() const { return false; }
bool Processor::isMidiEffect() const { return false; }
double Processor::getTailLengthSeconds() const { return 0.0; }
int Processor::getNumPrograms() { return 1; }
int Processor::getCurrentProgram() { return 0; }
void Processor::setCurrentProgram(int) {}
const juce::String Processor::getProgramName(int) { return {}; }
void Processor::changeProgramName(int, const juce::String &) {}

void Processor::prepareToPlay(double sampleRate, int)
{
  std::lock_guard<std::mutex> guard(setupMutex);

  if (previousSampleRate != sampleRate) {
    dsp.setup(sampleRate);
  } else {
    dsp.reset();
  }
  mpeInstrument.releaseAllNotes();
  setLatencySamples(int(dsp.getLatency()));
  previousSampleRate = sampleRate;
}

void Processor::releaseResources() {}

bool Processor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
  if (
    layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
    && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
  {
    return false;
  }

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;

  return true;
}

void Processor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
  // I guess mutex shouldn't be used here. However, this is a mitigation of a crash on FL
  // when refreshing a plugin. It seems like `prepareToPlay` and `processBlock` might be
  // called at the same time.
  std::lock_guard<std::mutex> guard(setupMutex);

  juce::ScopedNoDenormals noDenormals;

  for (const juce::MidiMessageMetadata &metadata : midi) {
    if (metadata.data == nullptr) continue;
    if (metadata.numBytes <= 0) continue;
    midiSampleOffset = metadata.samplePosition; // Sad hack to propagate message timing.
    mpeInstrument.processNextMidiEvent(metadata.getMessage());
  }

  auto audioPlayHead = getPlayHead();
  if (audioPlayHead != nullptr) {
    auto positionInfo = audioPlayHead->getPosition();
    if (positionInfo) {
      dsp.isPlaying = positionInfo->getIsPlaying();

      auto bpm = positionInfo->getBpm();
      if (bpm) dsp.tempo = *bpm;

      auto beats = positionInfo->getPpqPosition();
      if (beats) dsp.beatsElapsed = *beats;

      auto timeSignature = positionInfo->getTimeSignature();
      if (timeSignature) {
        dsp.timeSigUpper = timeSignature->numerator;
        dsp.timeSigLower = timeSignature->denominator;
      }
    }
  }

  for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  dsp.setParameters();

  auto inCar0 = buffer.getReadPointer(0);
  auto inCar1 = buffer.getReadPointer(1);
  auto inMod0 = buffer.getReadPointer(2);
  auto inMod1 = buffer.getReadPointer(3);
  auto out0 = buffer.getWritePointer(0);
  auto out1 = buffer.getWritePointer(1);
  dsp.process((size_t)buffer.getNumSamples(), inCar0, inCar1, inMod0, inMod1, out0, out1);
}

bool Processor::hasEditor() const { return true; }

juce::AudioProcessorEditor *Processor::createEditor()
{
  return new Uhhyou::Editor(*this);
}

void Processor::getStateInformation(juce::MemoryBlock &destData)
{
  auto state = param.tree.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void Processor::setStateInformation(const void *data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(param.tree.state.getType())) {
      param.tree.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
  }
}

void Processor::noteAdded(juce::MPENote) {}
void Processor::noteReleased(juce::MPENote) {}
void Processor::notePressureChanged(juce::MPENote) {}
void Processor::notePitchbendChanged(juce::MPENote) {}
void Processor::noteTimbreChanged(juce::MPENote) {}
void Processor::noteKeyStateChanged(juce::MPENote) {}
void Processor::zoneLayoutChanged() {}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new Processor(); }
