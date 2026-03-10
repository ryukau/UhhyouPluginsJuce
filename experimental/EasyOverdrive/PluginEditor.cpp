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

Editor::Editor(Processor& processor)
    : EditorBase(processor, informationText),
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

  layoutActionSectionAndPluginInfo(left1, currentTop, mt.sectionWidth, mt.labelW, mt.labelX,
                                   mt.labelH, mt.labelY, scale);

  statusBar.setBounds(
    Rect{left0, bottom - mt.labelH - mt.uiMargin, mt.totalWidth - 2 * mt.uiMargin, mt.labelH});
}

} // namespace Uhhyou
