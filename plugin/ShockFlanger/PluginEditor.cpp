// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.hpp"
#include "PluginProcessor.hpp"

#include "./gui/popupinformationtext.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <string>

namespace Uhhyou {

struct Metrics {
  int margin = 5;
  int labelH = 20;
  int labelW = 100;
  int uiMargin = 4 * margin;
  int labelX = labelW + 2 * margin;
  int labelY = labelH + 2 * margin;
  int sectionWidth = 2 * labelW + 2 * margin;
  int xypadWidth = sectionWidth * 2 / 3;
  int totalWidth = (uiMargin * 5 / 2) + (2 * xypadWidth + 2 * margin) + (6 * labelX);
  int totalHeight = 22 * labelY;

  Metrics() = default;

  explicit Metrics(float scale) {
    auto zoom = [scale](int val) { return static_cast<int>(std::lround(val * scale)); };

    // TODO: C++26 static reflection can be used to simplify the loop below.
    for (auto ptr : {&Metrics::margin, &Metrics::labelH, &Metrics::labelW, &Metrics::uiMargin,
                     &Metrics::labelX, &Metrics::labelY, &Metrics::sectionWidth,
                     &Metrics::xypadWidth, &Metrics::totalWidth, &Metrics::totalHeight})
    {
      this->*ptr = zoom(this->*ptr);
    }
  }
};

const juce::String sXY0{"XY-0"};
const juce::String sXY1{"XY-1"};
const juce::String sMix{"Mix"};
const juce::String sSat{"Saturation"};
const juce::String sDelay{"Delay"};
const juce::String sLFO{"LFO"};
const juce::String sMod{"Modulation"};
const juce::String sMIDI{"MIDI"};

Editor::Editor(Processor& processor) : EditorBase(processor, informationText) {
  auto& s_ = processor.param.scale;

  auto snapsToNormalized = [&](const auto& source, auto converter) {
    std::vector<float> keys;
    keys.reserve(source.size());
    std::transform(source.begin(), source.end(), std::back_inserter(keys),
                   [&](const auto& pair) { return converter(pair.first); });
    return keys;
  };

  addTextKnob(sMix, "dryGain", s_.gain,
              {s_.gain.invmap(-1.0f), s_.gain.invmap(0), s_.gain.invmap(1.0f)}, 5);
  addTextKnob(sMix, "wetGain", s_.gain,
              {s_.gain.invmap(-1.0f), s_.gain.invmap(0), s_.gain.invmap(1.0f)}, 5);
  addToggleButton(sMix, "wetInvert", s_.boolean);
  addToggleButton(sMix, "oversampling", s_.boolean);

  auto formatSaturationType = [](std::string s) {
    if (!s.empty()) {
      s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
      std::replace(s.begin(), s.end(), '_', ' ');
    }
    return s;
  };
  addComboBox(sSat, "saturationType", s_.saturationType,
              {
#define X(name) formatSaturationType(#name),
                UHHYOU_SATURATOR_FUNCTIONS(X)
#undef X
              },
              "Type");
  addTextKnob(sSat, "saturationGain", s_.saturationGain, {s_.saturationGain.invmapDB(0)}, 5,
              "Gain");

  addTextKnob(sDelay, "delayTimeMs", s_.delayTimeMs, {s_.delayTimeMs.invmap(1.0f)}, 5);
  addTextKnob(sDelay, "delayTimeRatio", s_.unipolar,
              {s_.unipolar.invmap(1.0f / 2), s_.unipolar.invmap(1.0f / 3),
               s_.unipolar.invmap(1.0f / 4), s_.unipolar.invmap(1.0f / 5),
               s_.unipolar.invmap(1.0f / 6), s_.unipolar.invmap(1.0f / 7),
               s_.unipolar.invmap(1.0f / 8)},
              5);
  addTextKnob(sDelay, "inputBlend", s_.unipolar, {s_.unipolar.invmap(0.0f)}, 5);
  addTextKnob(sDelay, "flangeBlend", s_.unipolar, {s_.unipolar.invmap(0.0f)}, 5);
  addToggleButton(sDelay, "flangePolarity", s_.boolean);
  addTextKnob(sDelay, "feedback0", s_.feedback, {s_.feedback.invmap(0)}, 5);
  addTextKnob(sDelay, "feedback1", s_.feedback, {s_.feedback.invmap(0)}, 5);
  addToggleButton(sDelay, "safeFeedback", s_.boolean);
  addToggleButton(sDelay, "feedbackGate", s_.boolean);
  addTextKnob(sDelay, "lowpassCutoffHz", s_.cutoffHz, {s_.cutoffHz.invmap(10000.0f)}, 5);
  addTextKnob(sDelay, "highpassCutoffHz", s_.cutoffHz,
              {s_.cutoffHz.invmap(1.0f), s_.cutoffHz.invmap(5.0f), s_.cutoffHz.invmap(12.0f),
               s_.cutoffHz.invmap(20.0f)},
              5);

  addTextKnob(sLFO, "lfoBeat", s_.lfoBeat,
              snapsToNormalized(s_.lfoBeatSnaps, [&](auto v) { return s_.lfoBeat.invmap(v); }), 5);
  addComboBox(sLFO, "lfoSyncType", s_.lfoSyncType, {"Phase", "Frequency", "Off (Fixed to 120 BPM)"},
              "Tempo Sync.");
  std::vector<float> phaseSnaps{s_.unipolar.invmap(0.0f / 8), s_.unipolar.invmap(1.0f / 8),
                                s_.unipolar.invmap(2.0f / 8), s_.unipolar.invmap(3.0f / 8),
                                s_.unipolar.invmap(4.0f / 8), s_.unipolar.invmap(5.0f / 8),
                                s_.unipolar.invmap(6.0f / 8), s_.unipolar.invmap(7.0f / 8)};
  addRotaryTextKnob(sLFO, "lfoPhaseInitial", s_.unipolar, phaseSnaps, 5);
  addRotaryTextKnob(sLFO, "lfoPhaseStereoOffset", s_.unipolar, phaseSnaps, 5);
  addMomentaryButton(sLFO, "lfoPhaseReset", s_.boolean);

  addTextKnob(sMod, "lfoTimeMod0", s_.lfoToDelayTimeOctave, {s_.lfoToDelayTimeOctave.invmap(0)}, 5);
  addTextKnob(sMod, "lfoTimeMod1", s_.lfoToDelayTimeOctave, {s_.lfoToDelayTimeOctave.invmap(0)}, 5);
  addTextKnob(sMod, "modulationTracking", s_.unipolar, {s_.unipolar.invmap(1.0f)}, 5);
  addTextKnob(sMod, "sigModMode", s_.unipolar, {s_.unipolar.invmap(0.0f)}, 5);
  addTextKnob(sMod, "viscosityLowpassHz", s_.cutoffHz,
              {s_.cutoffHz.invmap(20.0f), s_.cutoffHz.invmap(200.0f), s_.cutoffHz.invmap(2000.0f),
               s_.cutoffHz.invmap(20000.0f)},
              5);
  addTextKnob(sMod, "sigTimeMod0", s_.timeModOctave, {s_.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "sigTimeMod1", s_.timeModOctave, {s_.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "sigAmpMod0", s_.unipolar, {s_.unipolar.invmap(0.0f)}, 5);
  addTextKnob(sMod, "sigAmpMod1", s_.unipolar, {s_.unipolar.invmap(0.0f)}, 5);

  addToggleButton(sMIDI, "noteReceive", s_.boolean);
  addTextKnob(sMIDI, "notePitchRange", s_.notePitchRange, {s_.notePitchRange.invmap(float(0))}, 5);
  addTextKnob(sMIDI, "noteGainRange", s_.noteGainRange,
              {s_.noteGainRange.invmap(float(10)), s_.noteGainRange.invmap(float(20)),
               s_.noteGainRange.invmap(float(30)), s_.noteGainRange.invmap(float(40)),
               s_.noteGainRange.invmap(float(50))},
              5);

  addXYPad(sXY0, "delayTimeMs", "delayTimeRatio", {s_.delayTimeMs.invmap(1.0f)},
           {s_.unipolar.invmap(0.75f)});
  addXYPad(sXY0, "feedback0", "feedback1", {s_.feedback.invmap(0)}, {s_.feedback.invmap(0)});
  addXYPad(sXY0, "lfoBeat", "saturationGain", {}, {s_.saturationGain.invmapDB(float(0))});
  addXYPad(sXY0, "flangeBlend", "inputBlend", {s_.unipolar.invmap(0.0f)},
           {s_.unipolar.invmap(0.0f)});

  addXYPad(sXY1, "lfoTimeMod0", "lfoTimeMod1", {s_.lfoToDelayTimeOctave.invmap(0)},
           {s_.lfoToDelayTimeOctave.invmap(0)});
  addXYPad(sXY1, "sigTimeMod0", "sigTimeMod1", {s_.timeModOctave.invmap(0)},
           {s_.timeModOctave.invmap(0)});
  addXYPad(sXY1, "sigAmpMod0", "sigAmpMod1", {s_.unipolar.invmap(0)}, {s_.unipolar.invmap(0)});
  addXYPad(sXY1, "sigModMode", "viscosityLowpassHz", {s_.unipolar.invmap(0)},
           {s_.cutoffHz.invmap(2000.0f)});

  // `setSize` must be called at last.
  const float scale = getWindowScale();
  Metrics mt{scale};
  initWindow(mt.totalWidth, mt.totalHeight);
}

void Editor::paint(juce::Graphics& ctx) { EditorBase::paint(ctx); }

void Editor::resized() {
  EditorBase<Processor>::resized();

  using Rect = juce::Rectangle<int>;

  const int defaultHeight = Metrics{1.0f}.totalHeight;
  const float scale = updateScale(defaultHeight);
  Metrics mt{scale};

  const int top0 = mt.uiMargin;

  const int left0 = mt.uiMargin;
  const int left1 = left0 + 2 * mt.labelX;
  const int left2 = left1 + 2 * mt.labelX;
  const int left3 = left2 + 2 * mt.labelX;

  const int leftXy0 = left3 + 2 * mt.margin;
  const int leftXy1 = leftXy0 + mt.xypadWidth + 2 * mt.margin;

  const int nameTop0 = layoutActionSectionAndPluginInfo(left0, top0, mt.sectionWidth, mt.labelW,
                                                        mt.labelX, mt.labelH, mt.labelY, scale);

  lfoPhaseDisplay.setBounds(Rect{left0, nameTop0 + mt.labelY, mt.labelW, mt.labelW});

  const int delayTimeTop = nameTop0 + mt.labelY + mt.labelW + 2 * mt.margin;
  delayTimeDisplay.setBounds(
    Rect{left0, delayTimeTop, mt.sectionWidth, 4 * mt.labelY - 2 * mt.margin});

  const int meterTop0 = delayTimeTop + 4 * mt.labelY;
  const int meterH = 6 * mt.labelY - 2 * mt.margin;
  meterPreSaturationPeak.setBounds(Rect{left0, meterTop0, mt.labelW, meterH});
  meterOutputPeak.setBounds(Rect{left0 + mt.labelX, meterTop0, mt.labelW, meterH});

  auto addSection = [&](int& top, int left, const juce::String& sectionTitle) {
    if (auto sc = sections.find(sectionTitle); sc != sections.end()) {
      top = layoutVerticalSection(groupLabels, left, top, mt.sectionWidth, mt.labelH, mt.labelY,
                                  sc->first, sc->second);
    }
  };
  int currentTop{};

  currentTop = top0;
  addSection(currentTop, left1, sMix);
  addSection(currentTop, left1, sSat);
  addSection(currentTop, left1, sDelay);
  statusBar.setBounds(Rect{left1, currentTop, mt.totalWidth - left1 - mt.uiMargin, mt.labelH});

  currentTop = top0;
  addSection(currentTop, left2, sLFO);
  addSection(currentTop, left2, sMod);
  addSection(currentTop, left2, sMIDI);

  currentTop = top0;
  if (auto sc = sections.find(sXY0); sc != sections.end()) {
    currentTop
      = layoutVerticalSection(groupLabels, leftXy0, currentTop, mt.xypadWidth, mt.xypadWidth,
                              mt.xypadWidth + 2 * mt.margin, "", sc->second);
  }

  currentTop = top0;
  if (auto sc = sections.find(sXY1); sc != sections.end()) {
    currentTop
      = layoutVerticalSection(groupLabels, leftXy1, currentTop, mt.xypadWidth, mt.xypadWidth,
                              mt.xypadWidth + 2 * mt.margin, "", sc->second);
  }
}

} // namespace Uhhyou
