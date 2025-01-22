// Copyright Takamitsu Endo (ryukau@gmail.com).
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.hpp"
#include "PluginProcessor.hpp"

#include "./gui/popupinformationtext.hpp"

#include <cmath>
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
  int totalWidth = 2 * sectionWidth + 3 * uiMargin;
  int totalHeight = 17 * labelY + labelH + 2 * uiMargin;

  Metrics() = default;

  explicit Metrics(float scale) {
    auto zoom = [scale](int val) { return static_cast<int>(std::lround(val * scale)); };

    for (auto ptr : {&Metrics::margin, &Metrics::labelH, &Metrics::labelW, &Metrics::uiMargin,
                     &Metrics::labelX, &Metrics::labelY, &Metrics::sectionWidth,
                     &Metrics::totalWidth, &Metrics::totalHeight})
    {
      this->*ptr = zoom(this->*ptr);
    }
  }
};

const juce::String sDrive{"Drive"};
const juce::String sAsym{"Asym. Drive"};
const juce::String sLimiter{"Limiter"};
const juce::String sMisc{"Misc."};

Editor::Editor(Processor& proc)
    : EditorBase(proc, informationText),
      oversamplingAttachment_(
        *proc.param.tree.getParameter("oversampling"),
        [this](float) { processor_.setLatencySamples(int(processor_.dsp.getLatency())); }, nullptr),
      limiterEnabledAttachment_(
        *proc.param.tree.getParameter("limiterEnabled"),
        [this](float) { processor_.setLatencySamples(int(processor_.dsp.getLatency())); },
        nullptr) {
  auto& sc = proc.param.scale;

  addTextKnob(sDrive, "preDriveGain", sc.gain, {}, 5);
  addTextKnob(sDrive, "postDriveGain", sc.gain, {}, 5);
  addComboBox(sDrive, "overDriveType", sc.overDriveType,
              {"Immediate", "HardGate", "Spike", "SpikeCubic", "CutoffMod", "Matched", "BadLimiter",
               "PolyDrive"},
              "");
  addTextKnob(sDrive, "overDriveHoldSecond", sc.overDriveHoldSecond, {}, 5);
  addTextKnob(sDrive, "overDriveQ", sc.filterQ, {}, 5);
  addTextKnob(sDrive, "overDriveCharacterAmp", sc.gain, {}, 5);

  addToggleButton(sAsym, "asymDriveEnabled", sc.boolean, "", "", LabeledWidget::expand);
  addTextKnob(sAsym, "asymDriveDecaySecond", sc.envelopeSecond, {}, 5);
  addTextKnob(sAsym, "asymDriveDecayBias", sc.asymDriveDecayBias, {}, 5);
  addTextKnob(sAsym, "asymDriveQ", sc.filterQ, {}, 5);
  addTextKnob(sAsym, "asymExponentRange", sc.asymExponentRange, {}, 5);

  addToggleButton(sLimiter, "limiterEnabled", sc.boolean, "", "", LabeledWidget::expand);
  addTextKnob(sLimiter, "limiterInputGain", sc.gain, {}, 5);
  addTextKnob(sLimiter, "limiterReleaseSecond", sc.envelopeSecond, {}, 5);

  addTextKnob(sMisc, "parameterSmoothingSecond", sc.parameterSmoothingSecond, {}, 5);
  addComboBox(sMisc, "oversampling", sc.oversampling, {"1x", "2x", "16x"}, "");

  // `setSize` must be called at last.
  const float scale = getWindowScale();
  Metrics mt{scale};
  initWindow(mt.totalWidth, mt.totalHeight);
}

void Editor::resized() {
  EditorBase<Processor>::resized();

  using Rect = juce::Rectangle<int>;

  const int defaultHeight = Metrics{1.0f}.totalHeight;
  const float scale = updateScale(defaultHeight);
  Metrics mt{scale};

  const int bottom = int(scale * defaultHeight);
  const int top0 = mt.uiMargin;
  const int left0 = mt.uiMargin;
  const int left1 = left0 + mt.sectionWidth + mt.uiMargin;

  int currentTop = top0;
  if (auto sc = sections_.find(sDrive); sc != sections_.end()) {
    currentTop = layoutVerticalSection(groupLabels_, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }
  if (auto sc = sections_.find(sAsym); sc != sections_.end()) {
    currentTop = layoutVerticalSection(groupLabels_, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }
  if (auto sc = sections_.find(sLimiter); sc != sections_.end()) {
    currentTop = layoutVerticalSection(groupLabels_, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }

  currentTop = top0;
  if (auto sc = sections_.find(sMisc); sc != sections_.end()) {
    currentTop = layoutVerticalSection(groupLabels_, left1, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }

  layoutActionSectionAndPluginInfo(left1, currentTop, mt.sectionWidth, mt.labelW, mt.labelX,
                                   mt.labelH, mt.labelY);

  statusBar_.setBounds(
    Rect{left0, bottom - mt.labelH - mt.uiMargin, mt.totalWidth - 2 * mt.uiMargin, mt.labelH});
}

} // namespace Uhhyou
