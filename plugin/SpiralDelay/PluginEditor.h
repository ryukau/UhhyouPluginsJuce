// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "Uhhyou/gui/widgets.hpp"

#include "PluginProcessor.h"

#include <array>
#include <vector>

namespace Uhhyou {

class Editor final : public juce::AudioProcessorEditor {
public:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)

  explicit Editor(Processor &);
  ~Editor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  virtual void mouseDown(const juce::MouseEvent &) override
  {
    numberEditor.setVisible(false);
  }

private: // JUCE related internals.
  Processor &processor;
  Palette palette;
  juce::LookAndFeel_V4 lookAndFeel;

  inline juce::ValueTree getStateTree()
  {
    return processor.param.tree.state.getOrCreateChildWithName("GUI", nullptr);
  }

private: // Action items.
  StatusBar statusBar;
  NumberEditor numberEditor;
  PopUpButton pluginNameButton;
  ActionButton<> undoButton;
  ActionButton<> redoButton;
  ActionButton<> randomizeButton;
  std::unique_ptr<juce::FileChooser> fileChooser;
  PresetManager presetManager;

private: // Controls tied to parameters.
  TextKnob<decltype(Scales::gain)> dryGain;
  TextKnob<decltype(Scales::gain)> wetGain;
  TextKnob<decltype(Scales::gain)> crossGain;
  TextKnob<decltype(Scales::saturationGain)> saturationGain;
  TextKnob<decltype(Scales::delayTimeMs)> delayTimeMs;
  TextKnob<decltype(Scales::unipolar)> delayTimeRatio;
  TextKnob<decltype(Scales::feedback)> feedback0;
  TextKnob<decltype(Scales::feedback)> feedback1;
  TextKnob<decltype(Scales::rotationHz)> rotationHz; // TODO: RotaryTextKnob
  TextKnob<decltype(Scales::unipolar)> stereoPhaseOffset;
  TextKnob<decltype(Scales::unipolar)> notchMix;
  TextKnob<decltype(Scales::notchWidth)> notchWidth;
  TextKnob<decltype(Scales::notchTracking)> notchTracking;
  TextKnob<decltype(Scales::highpassCutoffHz)> highpassCutoffHz;
  TextKnob<decltype(Scales::timeModOctave)> rotationToDelayTimeOctave0;
  TextKnob<decltype(Scales::timeModOctave)> rotationToDelayTimeOctave1;
  TextKnob<decltype(Scales::timeModOctave)> crossModulationOctave0;
  TextKnob<decltype(Scales::timeModOctave)> crossModulationOctave1;
  ToggleButton<decltype(Scales::boolean)> oversampling;

private: // Drawing items.
  std::vector<Line> lines;
  std::vector<TextLabel> labels;
  std::vector<GroupLabel> groupLabels;
};

} // namespace Uhhyou
