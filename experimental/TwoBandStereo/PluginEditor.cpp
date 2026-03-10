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
  int totalHeight = 10 * labelY;

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

const juce::String sStereoControl{"Stereo Control"};

Editor::Editor(Processor& processor) : EditorBase(processor, informationText) {
  auto& s_ = processor.param.scale;

  addTextKnob(sStereoControl, "crossoverHz", s_.crossoverHz, {}, 5);
  addTextKnob(sStereoControl, "upperStereoSpread", s_.unipolar, {}, 5);
  addTextKnob(sStereoControl, "lowerStereoSpread", s_.unipolar, {}, 5);

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
  if (auto sc = sections.find(sStereoControl); sc != sections.end()) {
    currentTop = layoutVerticalSection(groupLabels, left0, currentTop, mt.sectionWidth, mt.labelH,
                                       mt.labelY, sc->first, sc->second);
  }

  layoutActionSectionAndPluginInfo(left1, top0, mt.sectionWidth, mt.labelW, mt.labelX, mt.labelH,
                                   mt.labelY, scale);

  statusBar.setBounds(
    Rect{left0, bottom - mt.labelH - mt.uiMargin, mt.totalWidth - 2 * mt.uiMargin, mt.labelH});
}

} // namespace Uhhyou
