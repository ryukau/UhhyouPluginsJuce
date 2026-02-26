// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.hpp"
#include "PluginProcessor.hpp"

#include "./gui/popupinformationtext.hpp"
#include "Uhhyou/librarylicense.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <string>

namespace Uhhyou {

struct Metrics {
  static constexpr int baseMargin = 5;
  static constexpr int baseLabelHeight = 20;
  static constexpr int baseLabelWidth = 100;

  int margin{};
  int labelH{};
  int labelW{};
  int uiMargin{};
  int labelX{};
  int labelY{};
  int sectionWidth{};
  int xypadWidth{};
  int totalWidth{};
  int totalHeight{};

  Metrics(float scale) {
    // Intermediate values to prevent the truncation of fractions.
    const float f_margin = scale * baseMargin;
    const float f_labelH = scale * baseLabelHeight;
    const float f_labelW = scale * baseLabelWidth;
    const float f_uiMargin = 4 * f_margin;
    const float f_labelX = f_labelW + 2 * f_margin;
    const float f_labelY = f_labelH + 2 * f_margin;
    const float f_sectionWidth = 2 * f_labelW + 2 * f_margin;
    const float f_xypadWidth = f_sectionWidth * 2.0f / 3.0f;

    margin = int(f_margin);
    labelH = int(f_labelH);
    labelW = int(f_labelW);
    uiMargin = int(f_uiMargin);
    labelX = int(f_labelX);
    labelY = int(f_labelY);
    sectionWidth = int(f_sectionWidth);
    xypadWidth = int(f_xypadWidth);
    totalWidth = int(2 * f_xypadWidth + 3 * f_sectionWidth + 3 * f_uiMargin);
    totalHeight = int(26 * (f_labelH + 2 * f_margin));
  }
};

const juce::String sXY0{"XY-0"};
const juce::String sXY1{"XY-1"};
const juce::String sMix{"Mix"};
const juce::String sSat{"Saturation"};
const juce::String sDelay{"Delay"};
const juce::String sMod{"Modulation"};

Editor::Editor(Processor& processor)
    : EditorBase(processor, informationText, getLibraryLicenseText(), [&]() {
        std::uniform_real_distribution<float> dist{0.0f, 1.0f};
        std::random_device dev;
        std::mt19937 rng(dev());

        auto params = processor.getParameters();
        for (auto& prm : params) {
          if (this->paramLocks.isLocked(prm)) { continue; }

          float normalized = dist(rng);
          if (auto it = this->randomizers.find(prm); it != this->randomizers.end()) {
            normalized = it->second(normalized);
          }

          prm->beginChangeGesture();
          prm->setValueNotifyingHost(normalized);
          prm->endChangeGesture();
        }
      }) {
  auto& s_ = processor.param.scale;

  auto snapsToNormalized = [&](const auto& source, auto converter) {
    std::vector<float> keys;
    keys.reserve(source.size());
    std::transform(source.begin(), source.end(), std::back_inserter(keys),
                   [&](const auto& pair) { return converter(pair.first); });
    return keys;
  };

  addXYPad(sXY0, "delayTimeMs", "delayTimeRatio", {}, {});
  addXYPad(sXY0, "feedback0", "feedback1", {s_.feedback.invmap(0)}, {s_.feedback.invmap(0)});
  addXYPad(sXY0, "crossModulationOctave0", "crossModulationOctave1", {s_.timeModOctave.invmap(0)},
           {s_.timeModOctave.invmap(0)});

  addXYPad(sXY1, "flangeMode", "saturationGain", {}, {s_.saturationGain.invmap(1.0f)});
  addXYPad(sXY1, "lfoToDelayTimeOctave0", "lfoToDelayTimeOctave1",
           {s_.lfoToDelayTimeOctave.invmap(0)}, {s_.lfoToDelayTimeOctave.invmap(0)});
  addXYPad(sXY1, "inputRatio", "modulationTracking", {s_.unipolar.invmap(0.5f)},
           {s_.unipolar.invmap(0.5f)});

  addTextKnob(sMix, "dryGain", s_.gain,
              {s_.gain.invmap(-1.0f), s_.gain.invmap(0), s_.gain.invmap(1.0f)}, 5);
  addTextKnob(sMix, "wetGain", s_.gain,
              {s_.gain.invmap(-1.0f), s_.gain.invmap(0), s_.gain.invmap(1.0f)}, 5);
  addToggleButton(sMix, "wetInvert", s_.boolean);
  addToggleButton(sMix, "oversampling", s_.boolean);

  addComboBox(
    sSat, "saturationType", s_.saturationType,
    {
      "hardclip_cleaner",    "hardclip",    "softsign",    "softsign3",     "chebyshev_trig",
      "chebyshev_clenshaw",  "tanh",        "atan",        "expm1",         "log1p",
      "triangle_cleaner",    "triangle",    "modulo_sqrt", "modulo_linear", "modulo_linear2",
      "modulo_quad_cleaner", "modulo_quad", "sin_expm1",   "sin_growing",   "sin_growing2",
      "sin_stairs",          "versinc",
    },
    "Type");
  addTextKnob(sSat, "saturationGain", s_.saturationGain, {s_.saturationGain.invmapDB(0)}, 5,
              "Gain");

  addTextKnob(sDelay, "inputRatio", s_.unipolar, {s_.unipolar.invmap(0.5f)}, 5);
  addTextKnob(sDelay, "delayTimeMs", s_.delayTimeMs, {s_.delayTimeMs.invmap(1.0f)}, 5);
  addTextKnob(sDelay, "delayTimeRatio", s_.unipolar,
              {s_.unipolar.invmap(1.0f / 2), s_.unipolar.invmap(1.0f / 3),
               s_.unipolar.invmap(1.0f / 4), s_.unipolar.invmap(1.0f / 5),
               s_.unipolar.invmap(1.0f / 6), s_.unipolar.invmap(1.0f / 7),
               s_.unipolar.invmap(1.0f / 8)},
              5);
  addTextKnob(sDelay, "flangeMode", s_.unipolar, {}, 5);
  addToggleButton(sDelay, "safeFlange", s_.boolean);
  addToggleButton(sDelay, "safeFeedback", s_.boolean);
  addToggleButton(sDelay, "feedbackGate", s_.boolean);
  addTextKnob(sDelay, "feedback0", s_.feedback, {s_.feedback.invmap(0)}, 5);
  addTextKnob(sDelay, "feedback1", s_.feedback, {s_.feedback.invmap(0)}, 5);
  addTextKnob(sDelay, "lfoBeat", s_.lfoBeat,
              snapsToNormalized(s_.lfoBeatSnaps, [&](auto v) { return s_.lfoBeat.invmap(v); }), 5);

  std::vector<float> phaseSnaps{s_.unipolar.invmap(0.0f / 8), s_.unipolar.invmap(1.0f / 8),
                                s_.unipolar.invmap(2.0f / 8), s_.unipolar.invmap(3.0f / 8),
                                s_.unipolar.invmap(4.0f / 8), s_.unipolar.invmap(5.0f / 8),
                                s_.unipolar.invmap(6.0f / 8), s_.unipolar.invmap(7.0f / 8)};
  addRotaryTextKnob(sDelay, "lfoPhaseInitial", s_.unipolar, phaseSnaps, 5);
  addRotaryTextKnob(sDelay, "lfoPhaseStereoOffset", s_.unipolar, phaseSnaps, 5);
  addMomentaryButton(sDelay, "lfoPhaseReset", s_.boolean);

  addTextKnob(sDelay, "lowpassCutoffHz", s_.cutoffHz, {s_.cutoffHz.invmap(10000.0f)}, 5);
  addTextKnob(sDelay, "highpassCutoffHz", s_.cutoffHz,
              {s_.cutoffHz.invmap(1.0f), s_.cutoffHz.invmap(5.0f), s_.cutoffHz.invmap(12.0f),
               s_.cutoffHz.invmap(20.0f)},
              5);

  addToggleButton(sDelay, "noteReceive", s_.boolean);
  addTextKnob(sDelay, "notePitchRange", s_.notePitchRange, {s_.notePitchRange.invmap(float(0))}, 5);
  addTextKnob(sDelay, "noteGainRange", s_.noteGainRange,
              {s_.noteGainRange.invmap(float(10)), s_.noteGainRange.invmap(float(20)),
               s_.noteGainRange.invmap(float(30)), s_.noteGainRange.invmap(float(40)),
               s_.noteGainRange.invmap(float(50))},
              5);

  addTextKnob(sMod, "modulationTracking", s_.unipolar, {s_.unipolar.invmap(1.0f)}, 5);
  addTextKnob(sMod, "crossModMode", s_.unipolar, {s_.unipolar.invmap(0.5f)}, 5);
  addTextKnob(sMod, "crossModulationOctave0", s_.timeModOctave, {s_.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "crossModulationOctave1", s_.timeModOctave, {s_.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "lfoToDelayTimeOctave0", s_.lfoToDelayTimeOctave,
              {s_.lfoToDelayTimeOctave.invmap(0)}, 5);
  addTextKnob(sMod, "lfoToDelayTimeOctave1", s_.lfoToDelayTimeOctave,
              {s_.lfoToDelayTimeOctave.invmap(0)}, 5);

  // `setSize` must be called at last.
  const float scale = getStateTree().getProperty("windowScale", 1.0f);
  Metrics mt{scale};
  getConstrainer()->setFixedAspectRatio(double(mt.totalWidth) / double(mt.totalHeight));
  setSize(int(mt.totalWidth), int(mt.totalHeight));

  addAndMakeVisible(focusOverlay);
  focusOverlay.toFront(false);
}

void Editor::paint(juce::Graphics& ctx) { EditorBase::paint(ctx); }

void Editor::resized() {
  EditorBase<Processor>::resized();

  using Rect = juce::Rectangle<int>;

  const int defaultHeight = Metrics{1.0f}.totalHeight;
  const float scale = getHeight() / float(defaultHeight);
  getStateTree().setProperty("windowScale", scale, nullptr);
  palette.resize(scale);
  Metrics mt{scale};

  groupLabels.clear();

  const int bottom = int(scale * defaultHeight);
  const int top0 = mt.uiMargin;
  const int leftXy0 = mt.uiMargin;
  const int leftXy1 = leftXy0 + mt.xypadWidth;
  const int left1 = leftXy1 + mt.xypadWidth + 2 * mt.margin;
  const int left2 = left1 + 2 * mt.labelX;
  const int left3 = left2 + 2 * mt.labelX;

  auto addSection = [&](int& top, int left, const juce::String& sectionTitle) {
    if (auto sc = sections.find(sectionTitle); sc != sections.end()) {
      top = layoutVerticalSection(groupLabels, left, top, mt.sectionWidth, mt.labelH, mt.labelY,
                                  sc->first, sc->second);
    }
  };

  int currentTop = top0;
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

  currentTop = top0;
  addSection(currentTop, left1, sMix);
  addSection(currentTop, left1, sSat);
  addSection(currentTop, left1, sMod);

  currentTop = top0;
  addSection(currentTop, left2, sDelay);

  const int nameTop0 = layoutActionSection(groupLabels, left3, top0, mt.sectionWidth, mt.labelW,
                                           mt.labelX, mt.labelH, mt.labelY, undoButton, redoButton,
                                           randomizeButton, presetManager);
  pluginInfoButton.setBounds(Rect{left3, nameTop0, mt.sectionWidth, mt.labelH});
  pluginInfoButton.scale(scale);

  lfoPhaseDisplay.setBounds(Rect{left3, nameTop0 + mt.labelY, mt.labelW, mt.labelW});

  const int delayTimeTop = nameTop0 + mt.labelY + mt.labelW + 2 * mt.margin;
  delayTimeDisplay.setBounds(
    Rect{left3, delayTimeTop, mt.sectionWidth, 4 * mt.labelY - 2 * mt.margin});

  const int meterTop0 = delayTimeTop + 4 * mt.labelY;
  const int meterH = 6 * mt.labelY - 2 * mt.margin;
  meterPreSaturationPeak.setBounds(Rect{left3, meterTop0, mt.labelW, meterH});
  meterOutputPeak.setBounds(Rect{left3 + mt.labelX, meterTop0, mt.labelW, meterH});

  statusBar.setBounds(
    Rect{left1, bottom - mt.labelH - mt.uiMargin, 2 * mt.sectionWidth, mt.labelH});
}

} // namespace Uhhyou
