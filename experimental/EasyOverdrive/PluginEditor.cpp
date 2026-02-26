// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.hpp"
#include "PluginProcessor.hpp"

#include "Uhhyou/librarylicense.hpp"
#include "gui/popupinformationtext.hpp"

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
  int totalWidth{};
  int totalHeight{};

  Metrics(float scale) {
    const float f_margin = scale * baseMargin;
    const float f_labelH = scale * baseLabelHeight;
    const float f_labelW = scale * baseLabelWidth;
    const float f_uiMargin = 4 * f_margin;
    const float f_labelX = f_labelW + 2 * f_margin;
    const float f_labelY = f_labelH + 2 * f_margin;
    const float f_sectionWidth = 2 * f_labelW + 2 * f_margin;

    margin = int(f_margin);
    labelH = int(f_labelH);
    labelW = int(f_labelW);
    uiMargin = int(f_uiMargin);
    labelX = int(f_labelX);
    labelY = int(f_labelY);
    sectionWidth = int(f_sectionWidth);
    totalWidth = int(2 * f_sectionWidth + 3 * f_uiMargin);
    totalHeight = int(17 * f_labelY + f_labelH + 2 * f_uiMargin);
  }
};

const juce::String sDrive{"Drive"};
const juce::String sAsym{"Asym. Drive"};
const juce::String sLimiter{"Limiter"};
const juce::String sMisc{"Misc."};

Editor::Editor(Processor& processor)
    : EditorBase(processor, informationText, getLibraryLicenseText(),
                 [&]() {
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
                 }),
      oversamplingAttachment(
        *processor.param.tree.getParameter("oversampling"),
        [this](float) { this->processor.setLatencySamples(int(this->processor.dsp.getLatency())); },
        nullptr),
      limiterEnabledAttachment(
        *processor.param.tree.getParameter("limiterEnabled"),
        [this](float) { this->processor.setLatencySamples(int(this->processor.dsp.getLatency())); },
        nullptr) {
  auto& s_ = processor.param.scale;

  addTextKnob(sDrive, "preDriveGain", s_.gain, {}, 5);
  addTextKnob(sDrive, "postDriveGain", s_.gain, {}, 5);
  addComboBox(sDrive, "overDriveType", s_.overDriveType,
              {"Immediate", "HardGate", "Spike", "SpikeCubic", "CutoffMod", "Matched", "BadLimiter",
               "PolyDrive"},
              "");
  addTextKnob(sDrive, "overDriveHoldSecond", s_.overDriveHoldSecond, {}, 5);
  addTextKnob(sDrive, "overDriveQ", s_.filterQ, {}, 5);
  addTextKnob(sDrive, "overDriveCharacterAmp", s_.gain, {}, 5);

  addToggleButton(sAsym, "asymDriveEnabled", s_.boolean, "", "", LabeledWidget::expand);
  addTextKnob(sAsym, "asymDriveDecaySecond", s_.envelopeSecond, {}, 5);
  addTextKnob(sAsym, "asymDriveDecayBias", s_.asymDriveDecayBias, {}, 5);
  addTextKnob(sAsym, "asymDriveQ", s_.filterQ, {}, 5);
  addTextKnob(sAsym, "asymExponentRange", s_.asymExponentRange, {}, 5);

  addToggleButton(sLimiter, "limiterEnabled", s_.boolean, "", "", LabeledWidget::expand);
  addTextKnob(sLimiter, "limiterInputGain", s_.gain, {}, 5);
  addTextKnob(sLimiter, "limiterReleaseSecond", s_.envelopeSecond, {}, 5);

  addTextKnob(sMisc, "parameterSmoothingSecond", s_.parameterSmoothingSecond, {}, 5);
  addComboBox(sMisc, "oversampling", s_.oversampling, {"1x", "2x", "16x"}, "");

  // `setSize` must be called at last.
  const float scale = getStateTree().getProperty("windowScale", 1.0f);
  Metrics mt{scale};
  getConstrainer()->setFixedAspectRatio(double(mt.totalWidth) / double(mt.totalHeight));
  setSize(int(mt.totalWidth), int(mt.totalHeight));

  addAndMakeVisible(focusOverlay);
  focusOverlay.toFront(false);
}

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
  const int left0 = mt.uiMargin;
  const int left1 = left0 + mt.sectionWidth + mt.uiMargin;

  int currentTop = top0;
  if (auto sc = sections.find(sDrive); sc != sections.end()) {
    currentTop = layoutVerticalSection(groupLabels, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }
  if (auto sc = sections.find(sAsym); sc != sections.end()) {
    currentTop = layoutVerticalSection(groupLabels, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }
  if (auto sc = sections.find(sLimiter); sc != sections.end()) {
    currentTop = layoutVerticalSection(groupLabels, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }

  currentTop = top0;
  if (auto sc = sections.find(sMisc); sc != sections.end()) {
    currentTop = layoutVerticalSection(groupLabels, left1, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }

  const int nameTop0 = layoutActionSection(groupLabels, left1, currentTop, mt.sectionWidth,
                                           mt.labelW, mt.labelX, mt.labelH, mt.labelY, undoButton,
                                           redoButton, randomizeButton, presetManager);

  pluginInfoButton.setBounds(Rect{left1, nameTop0, mt.sectionWidth, mt.labelH});
  pluginInfoButton.scale(scale);

  statusBar.setBounds(
    Rect{left0, bottom - mt.labelH - mt.uiMargin, mt.totalWidth - 2 * mt.uiMargin, mt.labelH});
}

} // namespace Uhhyou
