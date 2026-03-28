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
  int labelW = 120;
  int uiMargin = 4 * margin;
  int labelX = labelW + 2 * margin;
  int labelY = labelH + 2 * margin;
  int sectionWidth = 2 * labelW + 2 * margin;
  int xypadWidth = 7 * labelH;
  int drawerButtonW = 0;

  int totalWidth = 0;
  int totalHeight = 22 * labelY;

  Metrics() = default;

  Metrics(float scale, bool isDrawerOpen) {
    auto zoom = [scale](int val) { return static_cast<int>(std::lround(val * scale)); };

    // TODO: C++26 static reflection can be used to simplify the loop below.
    for (auto ptr : {&Metrics::margin, &Metrics::labelH, &Metrics::labelW, &Metrics::uiMargin,
                     &Metrics::labelX, &Metrics::labelY, &Metrics::sectionWidth,
                     &Metrics::xypadWidth, &Metrics::drawerButtonW, &Metrics::totalHeight})
    {
      this->*ptr = zoom(this->*ptr);
    }
    drawerButtonW = HorizontalDrawer::calculateButtonWidth(scale);

    int left0 = uiMargin;
    int left3 = left0 + 6 * labelX;
    int drawerLeft = left3 + 2 * margin;
    int baseWidth = drawerLeft + drawerButtonW;
    int xypadAreaW = xypadWidth + 2 * margin + xypadWidth + 2 * uiMargin;

    if (isDrawerOpen) {
      totalWidth = baseWidth + xypadAreaW;
    } else {
      totalWidth = baseWidth;
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
const juce::String sToggles{"Toggles"};

Editor::Editor(Processor& proc) : EditorBase(proc, informationText) {
  auto& sc = proc.param.scale;

  auto snapsToNormalized = [&](const auto& source, auto converter) {
    std::vector<float> keys;
    keys.reserve(source.size());
    std::transform(source.begin(), source.end(), std::back_inserter(keys),
                   [&](const auto& pair) { return converter(pair.first); });
    return keys;
  };

  addToggleButton(sToggles, "wetInvert", sc.boolean);
  addToggleButton(sToggles, "oversampling", sc.boolean);
  addToggleButton<Style::warning>(sToggles, "moreFeedback", sc.boolean);
  addToggleButton(sToggles, "feedbackGate", sc.boolean);

  addTextKnob(sMix, "dryGain", sc.gain,
              {sc.gain.invmap(-1.0f), sc.gain.invmap(0), sc.gain.invmap(1.0f)}, 5);
  addTextKnob(sMix, "wetGain", sc.gain,
              {sc.gain.invmap(-1.0f), sc.gain.invmap(0), sc.gain.invmap(1.0f)}, 5);

  auto formatSaturationType = [](std::string s) {
    if (!s.empty()) {
      s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
      std::replace(s.begin(), s.end(), '_', ' ');
    }
    return s;
  };
  addComboBox<Style::warning>(sSat, "saturationType", sc.saturationType,
                              {
#define X(name) formatSaturationType(#name),
                                UHHYOU_SATURATOR_FUNCTIONS(X)
#undef X
                              },
                              "Type");
  addTextKnob<Style::warning>(sSat, "saturationGain", sc.saturationGain,
                              {sc.saturationGain.invmapDB(0)}, 5, "Gain");

  addTextKnob(sDelay, "delayTimeMs", sc.delayTimeMs, {sc.delayTimeMs.invmap(1.0f)}, 5);
  addTextKnob(sDelay, "delayTimeRatio", sc.unipolar,
              {sc.unipolar.invmap(1.0f / 2), sc.unipolar.invmap(1.0f / 3),
               sc.unipolar.invmap(1.0f / 4), sc.unipolar.invmap(1.0f / 5),
               sc.unipolar.invmap(1.0f / 6), sc.unipolar.invmap(1.0f / 7),
               sc.unipolar.invmap(1.0f / 8)},
              5);
  addTextKnob(sDelay, "inputBlend", sc.unipolar, {sc.unipolar.invmap(0.5f)}, 5);
  addTextKnob<Style::warning>(sDelay, "flangeBlend", sc.unipolar, {sc.unipolar.invmap(0.0f)}, 5);
  addTextKnob(sDelay, "flangePolarity", sc.bipolar, {sc.bipolar.invmap(0.0f)}, 5);
  addTextKnob(sDelay, "feedback0", sc.feedback, {sc.feedback.invmap(0)}, 5);
  addTextKnob(sDelay, "feedback1", sc.feedback, {sc.feedback.invmap(0)}, 5);
  addTextKnob(sDelay, "lowpassCutoffHz", sc.cutoffHz, {sc.cutoffHz.invmap(10000.0f)}, 5);
  addTextKnob(sDelay, "highpassCutoffHz", sc.cutoffHz,
              {sc.cutoffHz.invmap(1.0f), sc.cutoffHz.invmap(5.0f), sc.cutoffHz.invmap(12.0f),
               sc.cutoffHz.invmap(20.0f)},
              5);

  addTextKnob(sLFO, "lfoBeat", sc.lfoBeat,
              snapsToNormalized(sc.lfoBeatSnaps, [&](auto v) { return sc.lfoBeat.invmap(v); }), 5);
  addComboBox(sLFO, "lfoSyncType", sc.lfoSyncType, {"Phase", "Frequency", "Off (Fixed to 120 BPM)"},
              "Tempo Sync.");
  std::vector<float> phaseSnaps{sc.unipolar.invmap(0.0f / 8), sc.unipolar.invmap(1.0f / 8),
                                sc.unipolar.invmap(2.0f / 8), sc.unipolar.invmap(3.0f / 8),
                                sc.unipolar.invmap(4.0f / 8), sc.unipolar.invmap(5.0f / 8),
                                sc.unipolar.invmap(6.0f / 8), sc.unipolar.invmap(7.0f / 8)};
  addRotaryTextKnob(sLFO, "lfoPhaseInitial", sc.unipolar, phaseSnaps, 5);
  addRotaryTextKnob(sLFO, "lfoPhaseStereoOffset", sc.unipolar, phaseSnaps, 5);
  addMomentaryButton(sLFO, "lfoPhaseReset", sc.boolean);

  addTextKnob(sMod, "lfoTimeMod0", sc.lfoToDelayTimeOctave, {sc.lfoToDelayTimeOctave.invmap(0)}, 5);
  addTextKnob(sMod, "lfoTimeMod1", sc.lfoToDelayTimeOctave, {sc.lfoToDelayTimeOctave.invmap(0)}, 5);
  addTextKnob(sMod, "modulationTracking", sc.unipolar, {sc.unipolar.invmap(1.0f)}, 5);
  addTextKnob<Style::warning>(sMod, "audioModMode", sc.unipolar, {sc.unipolar.invmap(0.0f)}, 5);
  addTextKnob<Style::warning>(sMod, "viscosityLowpassHz", sc.cutoffHz,
                              {sc.cutoffHz.invmap(20.0f), sc.cutoffHz.invmap(200.0f),
                               sc.cutoffHz.invmap(2000.0f), sc.cutoffHz.invmap(20000.0f)},
                              5);
  addTextKnob(sMod, "audioTimeMod0", sc.timeModOctave, {sc.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "audioTimeMod1", sc.timeModOctave, {sc.timeModOctave.invmap(0)}, 5);
  addTextKnob(sMod, "audioAmpMod0", sc.unipolar, {sc.unipolar.invmap(0.0f)}, 5);
  addTextKnob(sMod, "audioAmpMod1", sc.unipolar, {sc.unipolar.invmap(0.0f)}, 5);

  addToggleButton(sMIDI, "noteReceive", sc.boolean);
  addTextKnob(sMIDI, "notePitchRange", sc.notePitchRange, {sc.notePitchRange.invmap(float(0))}, 5);
  addTextKnob(sMIDI, "noteGainRange", sc.noteGainRange,
              {sc.noteGainRange.invmap(float(10)), sc.noteGainRange.invmap(float(20)),
               sc.noteGainRange.invmap(float(30)), sc.noteGainRange.invmap(float(40)),
               sc.noteGainRange.invmap(float(50))},
              5);

  addAndMakeVisible(drawer_);
  registerInteractive(drawer_.getToggleButton());

  addXYPad(sXY0, "delayTimeMs", "delayTimeRatio", {sc.delayTimeMs.invmap(1.0f)},
           {sc.unipolar.invmap(0.75f)});
  addXYPad(sXY0, "feedback0", "feedback1", {sc.feedback.invmap(0)}, {sc.feedback.invmap(0)});
  addXYPad(sXY0, "audioTimeMod0", "audioTimeMod1", {sc.timeModOctave.invmap(0)},
           {sc.timeModOctave.invmap(0)});
  addXYPad(sXY0, "audioAmpMod0", "audioAmpMod1", {sc.unipolar.invmap(0)}, {sc.unipolar.invmap(0)});

  addXYPad(sXY1, "lfoBeat", "saturationGain", {}, {sc.saturationGain.invmapDB(float(0))});
  addXYPad(sXY1, "lfoTimeMod0", "lfoTimeMod1", {sc.lfoToDelayTimeOctave.invmap(0)},
           {sc.lfoToDelayTimeOctave.invmap(0)});
  addXYPad(sXY1, "flangeBlend", "flangePolarity", {sc.unipolar.invmap(0.0f)},
           {sc.unipolar.invmap(0.0f)});
  addXYPad(sXY1, "audioModMode", "viscosityLowpassHz", {sc.unipolar.invmap(0)},
           {sc.cutoffHz.invmap(2000.0f)});

  registerDrawer(drawer_, "XYDrawerOpen", {sXY0, sXY1}, [](float s, bool open) {
    Metrics mt{s, open};
    return juce::Point<int>(mt.totalWidth, mt.totalHeight);
  });

  // `setSize` must be called at last.
  const float scale = getWindowScale();
  Metrics mt{scale, drawer_.isOpen()};
  initWindow(mt.totalWidth, mt.totalHeight);
}

void Editor::paint(juce::Graphics& ctx) { EditorBase::paint(ctx); }

void Editor::resized() {
  EditorBase<Processor>::resized();

  using Rect = juce::Rectangle<int>;

  const int defaultHeight = Metrics{}.totalHeight;
  const float scale = updateScale(defaultHeight);
  Metrics mt{scale, drawer_.isOpen()};

  const int top0 = mt.uiMargin;

  const int left0 = mt.uiMargin;
  const int left1 = left0 + 2 * mt.labelX;
  const int left2 = left1 + 2 * mt.labelX;
  const int left3 = left2 + 2 * mt.labelX;

  const int drawerLeft = left3 + 2 * mt.margin;

  const int nameTop0 = layoutActionSectionAndPluginInfo(left0, top0, mt.sectionWidth, mt.labelW,
                                                        mt.labelX, mt.labelH, mt.labelY);

  int currentLeft0Top = nameTop0 + mt.labelY;

  groupLabels_.emplace_back("", Rect{left0, currentLeft0Top, mt.sectionWidth, mt.labelH});
  currentLeft0Top += mt.labelY;

  lfoPhaseDisplay_.setBounds(Rect{left0, currentLeft0Top, mt.labelW, mt.labelW});
  if (auto sc = sections_.find(sToggles); sc != sections_.end()) {
    layoutVerticalSection(groupLabels_, left0 + mt.labelX, currentLeft0Top, mt.labelW, mt.labelH,
                          mt.labelY, "", sc->second);
  }

  const int meterTop = currentLeft0Top + mt.labelW + 2 * mt.margin;
  const int meterH = 8 * mt.labelY - 2 * mt.margin;
  meterPreSaturationPeak_.setBounds(Rect{left0, meterTop, mt.labelW, meterH});
  meterOutputPeak_.setBounds(Rect{left0 + mt.labelX, meterTop, mt.labelW, meterH});

  const int statusTop = meterTop + meterH + 2 * mt.margin;
  statusBar_.setBounds(Rect{left0, statusTop, mt.totalWidth - left0 - mt.uiMargin, mt.labelH});

  auto addSection = [&](int& top, int left, const juce::String& sectionTitle) {
    if (auto sc = sections_.find(sectionTitle); sc != sections_.end()) {
      top = layoutVerticalSection(groupLabels_, left, top, mt.sectionWidth, mt.labelH, mt.labelY,
                                  sc->first, sc->second);
    }
  };

  int currentTop{};

  currentTop = top0;
  addSection(currentTop, left1, sMix);
  addSection(currentTop, left1, sSat);
  addSection(currentTop, left1, sDelay);
  delayTimeDisplay_.setBounds(
    Rect{left1, currentTop, mt.sectionWidth, 4 * mt.labelY - 2 * mt.margin});

  currentTop = top0;
  addSection(currentTop, left2, sLFO);
  addSection(currentTop, left2, sMod);
  addSection(currentTop, left2, sMIDI);

  // Layout drawer & contents.
  drawer_.setBounds(Rect{drawerLeft, 0, mt.totalWidth - drawerLeft, mt.totalHeight});

  int localTop = top0;
  int localXy0 = mt.uiMargin;
  int localXy1 = localXy0 + mt.xypadWidth + 2 * mt.margin;

  if (auto sc = sections_.find(sXY0); sc != sections_.end()) {
    layoutVerticalSection(groupLabels_, localXy0, localTop, mt.xypadWidth, mt.xypadWidth,
                          mt.xypadWidth + 2 * mt.margin, "", sc->second);
  }

  localTop = top0;
  if (auto sc = sections_.find(sXY1); sc != sections_.end()) {
    layoutVerticalSection(groupLabels_, localXy1, localTop, mt.xypadWidth, mt.xypadWidth,
                          mt.xypadWidth + 2 * mt.margin, "", sc->second);
  }
}

} // namespace Uhhyou
